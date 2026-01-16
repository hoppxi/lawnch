#include "gfx.hpp"
#include "string.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#ifdef __linux__
#include <fontconfig/fontconfig.h>
#endif

namespace Lawnch::Gfx {

BLRgba32 toBLColor(const Config::Color &c) {
  return BLRgba32(c.r * 255, c.g * 255, c.b * 255, c.a * 255);
}

BLRoundRect rounded_rect(double x, double y, double width, double height,
                         double radius) {
  if (radius > width / 2.0)
    radius = width / 2.0;
  if (radius > height / 2.0)
    radius = height / 2.0;

  return BLRoundRect(x, y, width, height, radius);
}

BLFont get_font(const std::string &family, double size,
                const std::string &weight) {
  static std::map<std::string, BLFont> font_cache;
  static std::mutex cache_mutex;

  std::string cache_key = family + "_" + std::to_string(size) + "_" + weight;

  {
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = font_cache.find(cache_key);
    if (it != font_cache.end()) {
      return it->second;
    }
  }

  static std::map<std::string, BLFontFace> face_cache;
  std::string font_path;

  static FcConfig *config = nullptr;
  static std::once_flag config_flag;
  std::call_once(config_flag, []() { config = FcInitLoadConfigAndFonts(); });

  if (config) {
    FcPattern *pat = FcNameParse((const FcChar8 *)family.c_str());

    int fc_weight = FC_WEIGHT_REGULAR;

    try {
      int numeric_weight = std::stoi(weight);
      if (numeric_weight <= 100)
        fc_weight = FC_WEIGHT_THIN;
      else if (numeric_weight <= 200)
        fc_weight = FC_WEIGHT_EXTRALIGHT;
      else if (numeric_weight <= 300)
        fc_weight = FC_WEIGHT_LIGHT;
      else if (numeric_weight <= 400)
        fc_weight = FC_WEIGHT_REGULAR;
      else if (numeric_weight <= 500)
        fc_weight = FC_WEIGHT_MEDIUM;
      else if (numeric_weight <= 600)
        fc_weight = FC_WEIGHT_DEMIBOLD;
      else if (numeric_weight <= 700)
        fc_weight = FC_WEIGHT_BOLD;
      else if (numeric_weight <= 800)
        fc_weight = FC_WEIGHT_EXTRABOLD;
      else
        fc_weight = FC_WEIGHT_BLACK;
    } catch (...) {
      if (Str::iequals(weight, "thin"))
        fc_weight = FC_WEIGHT_THIN;
      else if (Str::iequals(weight, "extralight"))
        fc_weight = FC_WEIGHT_EXTRALIGHT;
      else if (Str::iequals(weight, "light"))
        fc_weight = FC_WEIGHT_LIGHT;
      else if (Str::iequals(weight, "book"))
        fc_weight = FC_WEIGHT_BOOK;
      else if (Str::iequals(weight, "regular") ||
               Str::iequals(weight, "normal"))
        fc_weight = FC_WEIGHT_REGULAR;
      else if (Str::iequals(weight, "medium"))
        fc_weight = FC_WEIGHT_MEDIUM;
      else if (Str::iequals(weight, "semibold") ||
               Str::iequals(weight, "demibold"))
        fc_weight = FC_WEIGHT_DEMIBOLD;
      else if (Str::iequals(weight, "bold"))
        fc_weight = FC_WEIGHT_BOLD;
      else if (Str::iequals(weight, "extrabold"))
        fc_weight = FC_WEIGHT_EXTRABOLD;
      else if (Str::iequals(weight, "black"))
        fc_weight = FC_WEIGHT_BLACK;
    }

    FcPatternAddInteger(pat, FC_WEIGHT, fc_weight);
    FcPatternAddDouble(pat, FC_SIZE, size);

    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    FcResult result;
    FcPattern *font_pat = FcFontMatch(config, pat, &result);
    if (font_pat) {
      FcChar8 *file = NULL;
      if (FcPatternGetString(font_pat, FC_FILE, 0, &file) == FcResultMatch) {
        font_path = (const char *)file;
      }

      FcPatternDestroy(font_pat);
    }
    FcPatternDestroy(pat);
  }

  BLFontFace face;

  {
    std::lock_guard<std::mutex> lock(cache_mutex);
    if (!font_path.empty()) {
      if (face_cache.find(font_path) != face_cache.end()) {
        face = face_cache[font_path];
      } else {
        BLResult err = face.create_from_file(font_path.c_str());
        if (err == BL_SUCCESS) {
          face_cache[font_path] = face;
        }
      }
    }
  }

  BLFont font;
  if (face.is_valid()) {
    font.create_from_face(face, (float)size);
  } else {
    BLFontFace defaultFace;
    font.create_from_face(defaultFace, (float)size);
  }

  {
    std::lock_guard<std::mutex> lock(cache_mutex);
    font_cache[cache_key] = font;
  }

  return font;
}

std::string truncate_text(const std::string &text, BLFont &font,
                          double max_width) {
  if (text.empty())
    return "";

  BLTextMetrics tm;
  BLGlyphBuffer gb;
  gb.set_utf8_text(text.c_str());
  font.shape(gb);
  font.get_text_metrics(gb, tm);

  if (tm.advance.x <= max_width) {
    return text;
  }

  std::string ellipsis = "...";
  BLGlyphBuffer gb_ellipsis;
  gb_ellipsis.set_utf8_text(ellipsis.c_str());
  font.shape(gb_ellipsis);
  BLTextMetrics tm_ellipsis;
  font.get_text_metrics(gb_ellipsis, tm_ellipsis);
  double ellipsis_width = tm_ellipsis.advance.x;

  if (ellipsis_width >= max_width) {
    return "...";
  }

  double available_width = max_width - ellipsis_width;

  size_t len = text.length();

  for (size_t i = 1; i < len; ++i) {
    if ((text[i] & 0xC0) == 0x80)
      continue;

    std::string temp = text.substr(0, i);
    gb.set_utf8_text(temp.c_str());
    font.shape(gb);
    font.get_text_metrics(gb, tm);

    if (tm.advance.x > available_width) {
      size_t prev = i - 1;
      while (prev > 0 && (text[prev] & 0xC0) == 0x80)
        prev--;

      return text.substr(0, prev) + ellipsis;
    }
  }

  return text + ellipsis;
}

} // namespace Lawnch::Gfx
