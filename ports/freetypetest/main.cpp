#include <ft2build.h>
#include <stdint.h>
#include FT_FREETYPE_H

#include <print>

int main(int argc, char** argv) {
    FT_Library library;
    FT_Face face;

    uint32_t codepoint = 'G';
    if (argc > 1)
        codepoint = argv[1][0];

    // Initialize the FreeType library
    if (FT_Init_FreeType(&library)) {
        std::print("Error: Could not initialize FreeType library\n");
        return 1;
    }

    // Load a font face
    // From FreeType Docs:
    //   A digital font is a data file that may contain one or more font faces.
    //   For each of these, it contains character images, character metrics, as
    //   well as other kind of information important to the layout of text and
    //   the processing of specific character encodings.
    const char* font_path = "/fs0/bin/Hack-Regular.ttf";
    if (FT_New_Face(library, font_path, 0, &face)) {
        std::print("Error: Could not load font from {}\n", font_path);
        FT_Done_FreeType(library);
        return 1;
    }

    // Set the font size (width, height in points, horizontal resolution, vertical resolution)
    // Setting width to 0 lets it automatically scale proportionally to the height
    constexpr auto pt = 8;
    FT_Set_Char_Size(face, 0, pt * 64, 300, 300);

    // Get glyph index for codepoint.
    // From FreeType Docs:
    //
    //   A font file contains a set of glyphs; each one can be stored as a
    //   bitmap, a vector representation, or any other scheme. ...
    //   These glyphs can be stored in any order in the font file, and are
    //   typically accessed through a simple glyph index.
    //
    //   A font file contains one or more tables, called character maps (also
    //   called 'charmaps' or 'cmaps'), which are used to convert character
    //   codes for a given encoding (e.g., ASCII, Unicode, Big5, ShiftJIS, etc.)
    //   into glyph indices relative to the font file.
    //
    // This converts the Unicode codepoint into the font-specific glyph index.
    FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);

    // Load the glyph for the character
    // This loads the "character image", whether it is a bitmap, set of
    // curves/vectors, etc.
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT)) {
        std::print("Error: Could not load glyph\n");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return 1;
    }

    // Render the glyph into an anti-aliased bitmap
    // This converts the font-specific glyph format into a "normalized"
    // pixel bitmap.
    if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
        std::print("Error: Could not render glyph\n");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return 1;
    }

    // Print the rendered bitmap to the console using ASCII characters
    FT_Bitmap bitmap = face->glyph->bitmap;
    for (size_t y = 0; y < bitmap.rows; ++y) {
        for (size_t x = 0; x < bitmap.width; ++x) {
            uint8_t pixel_value = bitmap.buffer[y * bitmap.pitch + x];
            // Display different shades based on pixel intensity
            if (pixel_value == 0)
                std::print(" ");
            else if (pixel_value < 26)
                std::print(".");
            else if (pixel_value < 51)
                std::print(",");
            else if (pixel_value < 76)
                std::print(":");
            else if (pixel_value < 102)
                std::print("-");
            else if (pixel_value < 128)
                std::print("=");
            else if (pixel_value < 152)
                std::print("+");
            else if (pixel_value < 178)
                std::print("*");
            else if (pixel_value < 205)
                std::print("#");
            else if (pixel_value < 232)
                std::print("%");
            else
                std::print("@");
        }
        std::print("\n");
    }

    // Clean up memory
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;
}
