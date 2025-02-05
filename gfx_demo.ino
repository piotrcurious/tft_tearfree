#include <TFT_eSPI.h>    // Include the graphics library
#include <math.h>        // Needed for sin() and cos()

TFT_eSPI tft = TFT_eSPI();  // Create TFT instance

// Define a simple structure for a point (using float for precision)
struct Point {
  float x;
  float y;
};

// Define the number of vertices in the vector shape.
// In this example, we'll use a square.
const int numPoints = 4;

// Define the vector shape (a square)
// These coordinates can be adjusted to suit your display and desired shape.
Point shapePoints[numPoints] = {
  {50, 50},
  {100, 50},
  {100, 100},
  {50, 100}
};

//---------------------------------------------------------
// Function: rotateShape
// Description:
//   Given an array of original points, this function computes the rotated
//   positions (stored in the 'rotated' array) by rotating each point around
//   the shape's centroid by the specified angle (in radians).
//---------------------------------------------------------
void rotateShape(const Point *original, Point *rotated, int count, float angle) {
  // Compute the centroid of the shape.
  float cx = 0, cy = 0;
  for (int i = 0; i < count; i++) {
    cx += original[i].x;
    cy += original[i].y;
  }
  cx /= count;
  cy /= count;

  // Rotate each point around the centroid.
  for (int i = 0; i < count; i++) {
    // Translate point to origin (centroid becomes (0,0))
    float dx = original[i].x - cx;
    float dy = original[i].y - cy;
    // Apply rotation matrix
    rotated[i].x = cx + (dx * cos(angle) - dy * sin(angle));
    rotated[i].y = cy + (dx * sin(angle) + dy * cos(angle));
  }
}

//---------------------------------------------------------
// Function: drawShape
// Description:
//   Draws a closed vector shape by connecting each point in the array.
//   The shape is drawn with the specified color.
//---------------------------------------------------------
void drawShape(const Point *points, int count, uint16_t color) {
  for (int i = 0; i < count; i++) {
    int next = (i + 1) % count;  // Wrap-around to connect the last point to the first
    tft.drawLine(points[i].x, points[i].y, points[next].x, points[next].y, color);
  }
}

void setup() {
  tft.init();              // Initialize TFT
  tft.setRotation(1);      // Set screen rotation if needed
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  static float angle = 0;  // Rotation angle in radians (increases over time)
  tft.fillScreen(TFT_BLACK);  // Clear the screen

  // Create an array to hold the rotated points.
  Point rotatedPoints[numPoints];
  // Rotate the original shape by the current angle.
  rotateShape(shapePoints, rotatedPoints, numPoints, angle);
  // Draw the rotated shape in white.
  drawShape(rotatedPoints, numPoints, TFT_WHITE);

  // Optionally, draw the rotation axis (the centroid) as a small circle.
  float cx = 0, cy = 0;
  for (int i = 0; i < numPoints; i++) {
    cx += shapePoints[i].x;
    cy += shapePoints[i].y;
  }
  cx /= numPoints;
  cy /= numPoints;
  tft.fillCircle(cx, cy, 3, TFT_RED); // Draw the centroid in red

  // Increment the angle for continuous rotation.
  angle += 0.05;  // Adjust rotation speed here
  if (angle > 2 * PI) {
    angle = 0;
  }

  delay(30);  // Adjust the delay for your desired frame rate
}
