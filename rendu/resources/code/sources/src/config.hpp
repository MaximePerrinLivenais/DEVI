#pragma once

extern int kDebugLevel;


/// Constants for text blocks in mm
/// \{
// Number of pixels between two consecutive baselines
extern float kLineHeight;

// Number of blank pixels (spacing) between two words
extern float kWordSpacing;

// Number of approximative pixels for the word "Word"
extern float kWordWidth;

// Number of pixels that separate two level-1 columns
extern float kColumnSpacing;
// \}

extern float kLineHorizontalSigma;
extern float kLineVerticalSigma;
extern float kAngleTolerance;

extern int kLayoutPageOpeningWidth;
extern int kLayoutPageOpeningHeight;
extern int kLayoutBlockOpeningWidth;
extern int kLayoutBlockOpeningHeight;
extern int kLayoutWhiteLevel;


extern int   kLayoutBlockMinHeight;
extern int   kLayoutBlockMinWidth;
extern float kLayoutBlockFillingRatio;
