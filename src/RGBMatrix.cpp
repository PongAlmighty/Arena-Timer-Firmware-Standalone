/**
 * Source code for interfacing with the WaveShare RGB Matrix Panel
 */

#include <RGBMatrix.h>
#include <Adafruit_Protomatter.h>
#include <Arduino.h>

namespace RGBMatrix
{
    // Waveshare RGB-Matrix-P5-64x32 pin connections

    static constexpr int A = 7;
    static constexpr int B = 10;
    static constexpr int C = 8;
    static constexpr int D = 11;

    static constexpr int R1 = 14;
    static constexpr int G1 = 28;
    static constexpr int B1 = 15;
    static constexpr int R2 = 5;
    static constexpr int G2 = 29;
    static constexpr int B2 = 6;

    static constexpr int CLK = 26;
    static constexpr int LAT = 12;
    static constexpr int OE = 27;

    uint8_t rgbPins[] = {R1, G1, B1, R2, G2, B2};
    uint8_t addrPins[] = {A, B, C, D};

    Adafruit_Protomatter matrix(
        64,       // matrix chain width
        6,        // bitDepth
        1,        // rgbCount
        rgbPins,  // rgbList
        4,        // addrCount
        addrPins, // addrList
        CLK,      // clockPin
        LAT,      // latchPin
        OE,       // oePin
        true      // doubleBuffer
    );

    void init()
    {
        // Initialization code for the RGB matrix
        // Initialize matrix...
        ProtomatterStatus status = matrix.begin();
        Serial.print("Protomatter begin() status: ");
        Serial.println((int)status);
        if (status != PROTOMATTER_OK)
        {
            for (;;)
                ;
        }
    }

    void demo()
    {
        // Demo code for the RGB matrix
        // Make four color bars (red, green, blue, white) with brightness ramp:
        for (int x = 0; x < matrix.width(); x++)
        {
            uint8_t level = x * 256 / matrix.width(); // 0-255 brightness
            matrix.drawPixel(x, matrix.height() - 4, matrix.color565(level, 0, 0));
            matrix.drawPixel(x, matrix.height() - 3, matrix.color565(0, level, 0));
            matrix.drawPixel(x, matrix.height() - 2, matrix.color565(0, 0, level));
            matrix.drawPixel(x, matrix.height() - 1, matrix.color565(level, level, level));
        }

        // Simple shapes and text, showing GFX library calls:
        matrix.drawCircle(12, 10, 9, matrix.color565(255, 0, 0));               // Red
        matrix.drawRect(14, 6, 17, 17, matrix.color565(0, 255, 0));             // Green
        matrix.drawTriangle(32, 9, 41, 27, 23, 27, matrix.color565(0, 0, 255)); // Blue

        const char *text = "3:00";
        int16_t x1, y1;
        uint16_t w, h;
        matrix.getTextBounds((char *)text, 0, 0, &x1, &y1, &w, &h);

        // Compute centered position
        int16_t cx = (matrix.width() - w) / 2;
        int16_t cy = (matrix.height() - h) / 2;

        matrix.setCursor(cx, cy); // top-left corner (in rotated space)
        matrix.println(text); // Default text color is white

        // AFTER DRAWING, A show() CALL IS REQUIRED TO UPDATE THE MATRIX!

        matrix.show(); // Copy data to matrix buffers
    }

    // simple wrapper function to set display rotation based on degrees
    void setOrientation(int orientation)
    {
        switch (orientation)
        {
        case 0:
            matrix.setRotation(0);
            break;
        case 90:
            matrix.setRotation(1);
            break;
        case 180:
            matrix.setRotation(2);
            break;
        case 270:
            matrix.setRotation(3);
            break;
        }
    }

    void clear()
    {
        matrix.fillScreen(0);
    }

    void show()
    {
        matrix.show();
    }

    Adafruit_Protomatter& getMatrix()
    {
        return matrix;
    }
}