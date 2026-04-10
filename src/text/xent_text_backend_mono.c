#include "../xent_internal.h"

typedef struct XentDecodedGlyph {
    uint32_t codepoint;
    uint32_t cluster;
    uint8_t break_after;
} XentDecodedGlyph;

static bool xent_utf8_next(const char *text, size_t text_len, size_t *cursor, uint32_t *out_codepoint) {
    if (!text || !cursor || !out_codepoint || *cursor >= text_len) {
        return false;
    }

    const uint8_t *s = (const uint8_t *)text;
    uint8_t b0 = s[*cursor];
    if ((b0 & 0x80u) == 0u) {
        *out_codepoint = (uint32_t)b0;
        *cursor += 1u;
        return true;
    }

    if ((b0 & 0xE0u) == 0xC0u && (*cursor + 1u) < text_len) {
        uint8_t b1 = s[*cursor + 1u];
        if ((b1 & 0xC0u) == 0x80u) {
            *out_codepoint = ((uint32_t)(b0 & 0x1Fu) << 6u) | (uint32_t)(b1 & 0x3Fu);
            *cursor += 2u;
            return true;
        }
    } else if ((b0 & 0xF0u) == 0xE0u && (*cursor + 2u) < text_len) {
        uint8_t b1 = s[*cursor + 1u];
        uint8_t b2 = s[*cursor + 2u];
        if ((b1 & 0xC0u) == 0x80u && (b2 & 0xC0u) == 0x80u) {
            *out_codepoint =
                ((uint32_t)(b0 & 0x0Fu) << 12u) | ((uint32_t)(b1 & 0x3Fu) << 6u) | (uint32_t)(b2 & 0x3Fu);
            *cursor += 3u;
            return true;
        }
    } else if ((b0 & 0xF8u) == 0xF0u && (*cursor + 3u) < text_len) {
        uint8_t b1 = s[*cursor + 1u];
        uint8_t b2 = s[*cursor + 2u];
        uint8_t b3 = s[*cursor + 3u];
        if ((b1 & 0xC0u) == 0x80u && (b2 & 0xC0u) == 0x80u && (b3 & 0xC0u) == 0x80u) {
            *out_codepoint = ((uint32_t)(b0 & 0x07u) << 18u) | ((uint32_t)(b1 & 0x3Fu) << 12u) |
                             ((uint32_t)(b2 & 0x3Fu) << 6u) | (uint32_t)(b3 & 0x3Fu);
            *cursor += 4u;
            return true;
        }
    }

    *out_codepoint = 0xFFFDu;
    *cursor += 1u;
    return true;
}

static bool xent_is_break_opportunity(uint32_t codepoint) {
    switch (codepoint) {
        case (uint32_t)' ':
        case (uint32_t)'\t':
        case (uint32_t)'-':
        case (uint32_t)',':
        case (uint32_t)'.':
        case (uint32_t)';':
        case (uint32_t)':':
        case (uint32_t)'/':
        case 0x00B7u: /* middle dot */
        case 0x200Bu: /* zero-width space */
        case 0x3001u: /* ideographic comma */
        case 0x3002u: /* ideographic full stop */
        case 0xFF0Cu: /* fullwidth comma */
        case 0xFF0Eu: /* fullwidth full stop */
            return true;
        default:
            break;
    }
    return false;
}

static bool xent_decode_glyphs(const char *text, XentDecodedGlyph **out_glyphs, uint32_t *out_count) {
    *out_glyphs = NULL;
    *out_count = 0u;

    const size_t text_len = strlen(text);
    if (text_len == 0u) {
        return true;
    }

    uint32_t capacity = 64u;
    XentDecodedGlyph *glyphs = (XentDecodedGlyph *)malloc(sizeof(XentDecodedGlyph) * (size_t)capacity);
    if (!glyphs) {
        return false;
    }

    size_t cursor = 0u;
    uint32_t count = 0u;
    uint32_t codepoint = 0u;
    while (cursor < text_len) {
        size_t cluster = cursor;
        if (!xent_utf8_next(text, text_len, &cursor, &codepoint)) {
            break;
        }
        if (count == capacity) {
            uint32_t next_capacity = capacity * 2u;
            XentDecodedGlyph *next = (XentDecodedGlyph *)realloc(glyphs, sizeof(XentDecodedGlyph) * (size_t)next_capacity);
            if (!next) {
                free(glyphs);
                return false;
            }
            glyphs = next;
            capacity = next_capacity;
        }
        glyphs[count].codepoint = codepoint;
        glyphs[count].cluster = (uint32_t)cluster;
        glyphs[count].break_after = xent_is_break_opportunity(codepoint) ? 1u : 0u;
        count += 1u;
    }

    *out_glyphs = glyphs;
    *out_count = count;
    return true;
}

static uint32_t xent_resolve_max_glyphs_per_line(float glyph_w,
                                                 float width_constraint,
                                                 XentLineBreakPolicy line_break_policy,
                                                 XentMeasureMode width_mode) {
    if (width_mode == XENT_MEASURE_UNDEFINED || line_break_policy == XENT_LINE_BREAK_NO_WRAP ||
        !isfinite(width_constraint) || width_constraint <= 0.0f) {
        return UINT32_MAX;
    }
    uint32_t fit = (uint32_t)(width_constraint / glyph_w);
    return fit == 0u ? 1u : fit;
}

static bool xent_build_lines_char_wrap(uint32_t glyph_count,
                                       uint32_t max_glyphs_per_line,
                                       uint32_t *line_starts,
                                       uint32_t *line_counts,
                                       uint32_t *out_line_count) {
    uint32_t line_count = 0u;
    uint32_t cursor = 0u;
    while (cursor < glyph_count) {
        uint32_t count = glyph_count - cursor;
        if (count > max_glyphs_per_line) {
            count = max_glyphs_per_line;
        }
        line_starts[line_count] = cursor;
        line_counts[line_count] = count;
        line_count += 1u;
        cursor += count;
    }
    *out_line_count = line_count;
    return true;
}

static bool xent_build_lines_word_wrap(const XentDecodedGlyph *glyphs,
                                       uint32_t glyph_count,
                                       uint32_t max_glyphs_per_line,
                                       uint32_t *line_starts,
                                       uint32_t *line_counts,
                                       uint32_t *out_line_count) {
    uint32_t line_count = 0u;
    uint32_t line_start = 0u;

    while (line_start < glyph_count) {
        uint32_t scan = line_start;
        uint32_t last_break = UINT32_MAX;
        while (scan < glyph_count && (scan - line_start) < max_glyphs_per_line) {
            if (glyphs[scan].break_after != 0u) {
                last_break = scan;
            }
            scan += 1u;
        }

        uint32_t line_end = scan;
        if (scan < glyph_count && (scan - line_start) >= max_glyphs_per_line && last_break != UINT32_MAX &&
            last_break >= line_start) {
            line_end = last_break + 1u;
        }
        if (line_end <= line_start) {
            line_end = line_start + 1u;
        }

        line_starts[line_count] = line_start;
        line_counts[line_count] = line_end - line_start;
        line_count += 1u;
        line_start = line_end;
    }

    *out_line_count = line_count;
    return true;
}

static bool xent_shape_mono(const XentTextBackend *backend,
                            const char *text,
                            float font_size,
                            float width_constraint,
                            XentLineBreakPolicy line_break_policy,
                            XentMeasureMode width_mode,
                            XentShapedGlyph *out_glyphs,
                            uint32_t glyph_capacity,
                            XentShapedRun *out_runs,
                            uint32_t run_capacity,
                            XentShapedLine *out_lines,
                            uint32_t line_capacity,
                            XentShapingResult *out_result) {
    (void)font_size;
    if (!backend || !text || !out_result) {
        return false;
    }
    if ((out_glyphs == NULL && glyph_capacity != 0u) || (out_runs == NULL && run_capacity != 0u) ||
        (out_lines == NULL && line_capacity != 0u)) {
        return false;
    }
    if (line_break_policy != XENT_LINE_BREAK_NO_WRAP && line_break_policy != XENT_LINE_BREAK_WORD_WRAP &&
        line_break_policy != XENT_LINE_BREAK_CHAR_WRAP) {
        return false;
    }
    if (width_mode != XENT_MEASURE_UNDEFINED && width_mode != XENT_MEASURE_AT_MOST &&
        width_mode != XENT_MEASURE_EXACTLY) {
        return false;
    }

    const XentMonoBackendState *state = (const XentMonoBackendState *)backend->userdata;
    if (!state) {
        return false;
    }

    XentDecodedGlyph *glyphs = NULL;
    uint32_t glyph_count = 0u;
    if (!xent_decode_glyphs(text, &glyphs, &glyph_count)) {
        return false;
    }

    const float glyph_w = state->glyph_width;
    const float line_h = state->line_height;
    const uint32_t max_glyphs_per_line =
        xent_resolve_max_glyphs_per_line(glyph_w, width_constraint, line_break_policy, width_mode);

    uint32_t required_lines = 1u;
    uint32_t *line_starts = NULL;
    uint32_t *line_counts = NULL;
    if (glyph_count > 0u) {
        line_starts = (uint32_t *)malloc(sizeof(uint32_t) * (size_t)glyph_count);
        line_counts = (uint32_t *)malloc(sizeof(uint32_t) * (size_t)glyph_count);
        if (!line_starts || !line_counts) {
            free(glyphs);
            free(line_starts);
            free(line_counts);
            return false;
        }

        if (max_glyphs_per_line == UINT32_MAX) {
            line_starts[0] = 0u;
            line_counts[0] = glyph_count;
            required_lines = 1u;
        } else if (line_break_policy == XENT_LINE_BREAK_WORD_WRAP) {
            xent_build_lines_word_wrap(glyphs, glyph_count, max_glyphs_per_line, line_starts, line_counts, &required_lines);
        } else {
            xent_build_lines_char_wrap(glyph_count, max_glyphs_per_line, line_starts, line_counts, &required_lines);
        }
    }

    uint32_t run_count = glyph_count > 0u ? 1u : 0u;
    out_result->glyph_count = glyph_count;
    out_result->run_count = run_count;
    out_result->line_count = required_lines;
    out_result->truncated = false;

    float max_line_width = 0.0f;
    if (glyph_count > 0u) {
        for (uint32_t i = 0; i < required_lines; ++i) {
            float line_width = glyph_w * (float)line_counts[i];
            if (line_width > max_line_width) {
                max_line_width = line_width;
            }
        }
    }

    if (width_mode == XENT_MEASURE_EXACTLY && isfinite(width_constraint) && width_constraint >= 0.0f) {
        out_result->metrics.width = width_constraint;
    } else {
        out_result->metrics.width = max_line_width;
    }
    out_result->metrics.height = line_h * (float)required_lines;
    out_result->metrics.line_count = required_lines;

    if ((out_glyphs && glyph_capacity < glyph_count) || (out_runs && run_capacity < run_count) ||
        (out_lines && line_capacity < required_lines)) {
        out_result->truncated = true;
        free(glyphs);
        free(line_starts);
        free(line_counts);
        return false;
    }

    if (out_lines) {
        if (glyph_count == 0u) {
            out_lines[0].glyph_start = 0u;
            out_lines[0].glyph_count = 0u;
            out_lines[0].width = 0.0f;
        } else {
            for (uint32_t i = 0; i < required_lines; ++i) {
                out_lines[i].glyph_start = line_starts[i];
                out_lines[i].glyph_count = line_counts[i];
                out_lines[i].width = glyph_w * (float)line_counts[i];
            }
        }
    }

    if (out_glyphs && glyph_count > 0u) {
        uint32_t line_index = 0u;
        uint32_t line_end = line_starts[0] + line_counts[0];
        for (uint32_t i = 0; i < glyph_count; ++i) {
            while (line_index + 1u < required_lines && i >= line_end) {
                line_index += 1u;
                line_end = line_starts[line_index] + line_counts[line_index];
            }

            uint32_t col = i - line_starts[line_index];
            out_glyphs[i].codepoint = glyphs[i].codepoint;
            out_glyphs[i].cluster = glyphs[i].cluster;
            out_glyphs[i].line_index = line_index;
            out_glyphs[i].advance = glyph_w;
            out_glyphs[i].offset_x = glyph_w * (float)col;
            out_glyphs[i].offset_y = line_h * (float)line_index;
        }
    }

    if (out_runs && run_count == 1u) {
        out_runs[0].glyph_start = 0u;
        out_runs[0].glyph_count = glyph_count;
        out_runs[0].line_start = 0u;
        out_runs[0].line_count = required_lines;
    }

    free(glyphs);
    free(line_starts);
    free(line_counts);
    return true;
}

static bool xent_measure_mono(const XentTextBackend *backend,
                              const char *text,
                              float font_size,
                              float width_constraint,
                              XentLineBreakPolicy line_break_policy,
                              XentMeasureMode width_mode,
                              XentTextMetrics *out_metrics) {
    XentShapingResult shaped = {0};
    if (!xent_shape_mono(backend,
                         text,
                         font_size,
                         width_constraint,
                         line_break_policy,
                         width_mode,
                         NULL,
                         0u,
                         NULL,
                         0u,
                         NULL,
                         0u,
                         &shaped)) {
        return false;
    }
    *out_metrics = shaped.metrics;
    return true;
}

bool xent_text_backend_mono_init(XentContext *ctx) {
    if (!ctx) {
        return false;
    }

    ctx->mono_state.glyph_width = ctx->config.mono_glyph_width;
    ctx->mono_state.line_height = ctx->config.mono_line_height;

    ctx->mono_backend.name = "mono_fallback";
    ctx->mono_backend.measure = xent_measure_mono;
    ctx->mono_backend.shape = xent_shape_mono;
    ctx->mono_backend.userdata = &ctx->mono_state;
    ctx->text_backend = &ctx->mono_backend;
    return true;
}
