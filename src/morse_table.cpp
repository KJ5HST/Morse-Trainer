#include "morse_table.h"

const int morseTreetop = 63;
const char morseTable[] PROGMEM =
    "*5*H*4*S***V*3*I***F***U?*_**2*E*&*L\"**R*+.****A***P@**W***J'1* *6-B*=*D*/"
    "*X***N***C;(!K***Y***T*7*Z**,G***Q***M:8*!***O*9***0*";

const int morseTableLength = (morseTreetop * 2) + 1;
const int morseTreeLevels = 6; // log2(64) = 6

int morseEncode(char ch, char* buf) {
    // Convert to uppercase
    if (ch > 96) ch -= 32;

    // Find character in morse table
    int i;
    for (i = 0; i < morseTableLength; i++) {
        if (pgm_read_byte(morseTable + i) == ch) break;
    }
    if (i >= morseTableLength) {
        buf[0] = '\0';
        return 0;
    }

    int morseTablePos = i + 1; // 1-based

    // Find level in binary tree
    int test;
    int startLevel;
    for (startLevel = 0; startLevel < morseTreeLevels; startLevel++) {
        test = (morseTablePos + (1 << startLevel)) % (2 << startLevel);
        if (test == 0) break;
    }
    int numSignals = morseTreeLevels - startLevel;

    if (numSignals <= 0) {
        // Space character
        buf[0] = ' ';
        buf[1] = '\0';
        return 1;
    }

    int pos = 0;
    int tPos = morseTablePos;
    for (int j = startLevel; j < morseTreeLevels; j++) {
        int add = (1 << j);
        test = (tPos + add) / (2 << j);
        if (test & 1) {
            tPos += add;
            buf[numSignals - 1 - pos++] = '.';
        } else {
            tPos -= add;
            buf[numSignals - 1 - pos++] = '-';
        }
    }
    buf[pos] = '\0';
    return numSignals;
}

char morseDecode(const char* pattern) {
    int pos = morseTreetop; // root of tree (0-based index 63)
    for (int i = 0; pattern[i]; i++) {
        int level = 0;
        int tmp = pos + 1; // convert to 1-based for tree math
        for (level = 0; level < morseTreeLevels; level++) {
            if (((tmp + (1 << level)) % (2 << level)) == 0) break;
        }
        // Can't descend further
        if (level == 0) return '\0';

        int step = (1 << (level - 1));
        if (pattern[i] == '.') {
            pos -= step;
        } else if (pattern[i] == '-') {
            pos += step;
        } else {
            return '\0';
        }
    }
    if (pos < 0 || pos >= morseTableLength) return '\0';
    char ch = pgm_read_byte(morseTable + pos);
    return (ch == '*') ? '\0' : ch;
}
