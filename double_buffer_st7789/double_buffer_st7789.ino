#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

#define TFT_WIDTH 240
#define TFT_HEIGHT 200 // 124k limit of esp32 dma
//#define DISPLAY_REFRESH_RATE 60.0

float   DISPLAY_REFRESH_RATE = 33.0;
#define TEARING_DEBUG // uncomment to adjust frame synchronization 
                      // 1-7 to choose parameter
                      // -/= keys to adjust parameter
                      // after adjusting write down parameters , disable debug and put them into code

#ifdef TEARING_DEBUG
uint8_t var1 = 0x1f; // frame 
uint8_t var2 = 0x16; // front porch
uint8_t var3 = 0x3f; // back porch

uint8_t var4 = 0x1f; // frame 
uint8_t var5 = 0x40; // front porch
uint8_t var6 = 0x3f;  // back porch

uint8_t selected = 7 ; 

void serial_tft_helper () {
    // Check if there is any serial input
  if (Serial.available() > 0) {
    // Read the incoming byte
    char input = Serial.read();

    // Check if the input is a digit from 1 to 6
    if (input >= '1' && input <= '7') {
      // Set the selected variable to the corresponding one
      selected = input - '0';
    }

    // Check if the input is a minus sign
    else if (input == '-') {
      // Decrement the selected variable by 1
      switch (selected) {
        case 1:
          var1--;
          break;
        case 2:
          var2--;
          break;
        case 3:
          var3--;
          break;
        case 4:
          var4--;
          break;
        case 5:
          var5--;
          break;
        case 6:
          var6--;
          break;
        case 7:
          DISPLAY_REFRESH_RATE=DISPLAY_REFRESH_RATE -1.0;
          break;
      }
    }

    // Check if the input is an equal sign
    else if (input == '=') {
      // Increment the selected variable by 1
      switch (selected) {
        case 1:
          var1++;
          break;
        case 2:
          var2++;
          break;
        case 3:
          var3++;
          break;
        case 4:
          var4++;
          break;
        case 5:
          var5++;
          break;
        case 6:
          var6++;
          break;
        case 7:
          DISPLAY_REFRESH_RATE=DISPLAY_REFRESH_RATE +1.0;
          break;

      }
    }

    // Print all the variables to the serial monitor
    Serial.print("1 RTNA1 = ");
    Serial.print(var1,HEX);
    Serial.print(", 2 Fporch1= ");
    Serial.print(var2,HEX);
    Serial.print(", 3 Bporch1 = ");
    Serial.print(var3,HEX);
    Serial.print(", 4 RTNA2 = ");
    Serial.print(var4,HEX);
    Serial.print(", 5 Fporch2 = ");
    Serial.print(var5,HEX);
    Serial.print(", 6 Bporch2 = ");
    Serial.print(var6,HEX);
    Serial.print(", 7 VSYNC = ");
    Serial.print(DISPLAY_REFRESH_RATE,2);

    // Print the selected variable with an asterisk
    Serial.print(", selected = ");
    Serial.print(selected);
    Serial.println("*");
  }
}

#endif//#ifdef TEARING_DEBUG


struct BufferState {
    bool ready;
    uint32_t frameCount;
};

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr[2] = {TFT_eSprite(&tft), TFT_eSprite(&tft)};

// Buffer management
uint16_t* sprPtr[2];
volatile uint8_t displayBuffer = 0;
volatile uint8_t drawBuffer = 1;
volatile BufferState bufferStates[2] = {{false, 0}, {false, 0}};

// Synchronization
SemaphoreHandle_t bufferMutex;
unsigned long lastDisplayTime = 0;

// Frame timing control
bool oddFrame = false;
bool oddFrameAdjusted = false;

// Drawing task
void drawGraphics(void* parameter) {
    while (true) {
        // Clear the drawing buffer
        if (drawBuffer==0) {
        spr[drawBuffer].fillSprite(TFT_RED);
        } else {
        spr[drawBuffer].fillSprite(TFT_BLUE);
          
        }
        // Your drawing code here
        spr[drawBuffer].fillRect(random(TFT_WIDTH), random(TFT_HEIGHT), 20, 20, TFT_CYAN);
        spr[drawBuffer].drawLine(random(TFT_WIDTH), 0, TFT_WIDTH, TFT_HEIGHT, TFT_GREEN);

        // Signal that drawing is complete
        xSemaphoreTake(bufferMutex, portMAX_DELAY);
        bufferStates[drawBuffer].ready = true;
        bufferStates[drawBuffer].frameCount++;
        xSemaphoreGive(bufferMutex);

        // Wait until the current drawing buffer is displayed
        while (drawBuffer == displayBuffer) {
            vTaskDelay(1);
        }

        // Switch to the other buffer for drawing
 //       drawBuffer = (drawBuffer == 0) ? 1 : 0;
//        Serial.println(drawBuffer);
        vTaskDelay(1);
    }
}

#define COLOR_DEPTH 16  // must be 16 for dma to work

void setup() {
    // Initialize display
    tft.init();
    tft.initDMA();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    Serial.begin(115200);
    // Create sprites and get pointers
//    for (int i = 0; i < 2; i++) {
//        spr[i].createSprite(TFT_WIDTH, TFT_HEIGHT);
//        spr[i].fillSprite(TFT_BLACK);
//        sprPtr[i] = (uint16_t*)spr[i].getPointer();
//    }

      // Define sprite colour depth
  spr[0].setColorDepth(COLOR_DEPTH);
  spr[1].setColorDepth(COLOR_DEPTH);

  // Create the 2 sprites for full screen framebuffer
  sprPtr[0] = (uint16_t*)spr[0].createSprite(TFT_WIDTH, TFT_HEIGHT);
  sprPtr[1] = (uint16_t*)spr[1].createSprite(TFT_WIDTH, TFT_HEIGHT);


    // Create synchronization primitive
    bufferMutex = xSemaphoreCreateMutex();

    // Initial display timing setup for even frames
    tft.startWrite();
    tft.writecommand(ST7789_FRCTR2);
    tft.writedata(0x0b);
    tft.writecommand(ST7789_PORCTRL);    
    tft.writedata(0x16);
    tft.writedata(0x3f);
    tft.writedata(0x01); //enable separate port control
    tft.writedata(0x33); // portch control in idle mode
    tft.writedata(0x33);// porch control in partial mode
    
    tft.endWrite();

    bufferStates[drawBuffer].ready = false; // mark as ready initally

    // Start drawing task
    xTaskCreatePinnedToCore(drawGraphics, "DrawGraphics", 4096, NULL, 1, NULL, 0);
    Serial.println("task created");

}

void loop() {

    #ifdef TEARING_DEBUG
  serial_tft_helper();  // use to solve st7735 tearing problem
    #endif// #ifdef TEARING_DEBUG

    unsigned long currentTime = millis();
    // Check if it's time for the next frame
    if (currentTime - lastDisplayTime >= 1000 / DISPLAY_REFRESH_RATE) {
        lastDisplayTime = currentTime;

        tft.startWrite();

#ifdef TEARING_DEBUG
        // Set frame timing for even frames
        tft.writecommand(ST7789_FRCTR2);
        tft.writedata(var1);
        tft.writecommand(ST7789_PORCTRL);        
        tft.writedata(var2);
        tft.writedata(var3);
        tft.writedata(0x01); //enable separate porch control
        tft.writedata(0x33); // porch control in idle mode
        tft.writedata(0x33);// porch control in partial mode

#else

        // Set frame timing for even frames
        tft.writecommand(ST7789_FRCTR2);
        tft.writedata(0x0b);
        tft.writecommand(ST7789_PORCTRL);        
        tft.writedata(0x16);
        tft.writedata(0x3f);
        tft.writedata(0x01); //enable separate port control
        tft.writedata(0x33); // portch control in idle mode
        tft.writedata(0x33);// porch control in partial mode
#endif //#ifdef TEARING_DEBUG

        oddFrame = !oddFrame;
        oddFrameAdjusted = false;

        xSemaphoreTake(bufferMutex, portMAX_DELAY);
        bool bufferReady = bufferStates[drawBuffer].ready;
        xSemaphoreGive(bufferMutex);

        // Only swap and display if the drawing buffer is ready
        if (bufferReady) {
            // Wait for any ongoing DMA transfer
            while (tft.dmaBusy()) {
                yield();
            }

//        Serial.println("buffer_ready");

            // Push the current drawing buffer to display
            tft.pushImageDMA(0, 0, TFT_WIDTH, TFT_HEIGHT, sprPtr[drawBuffer]);

            // Update buffer states
            xSemaphoreTake(bufferMutex, portMAX_DELAY);
            bufferStates[drawBuffer].ready = false;
            drawBuffer = (drawBuffer == 0) ? 1 : 0;
//            displayBuffer = drawBuffer;
            xSemaphoreGive(bufferMutex);
        }

        tft.endWrite();
    }

    // Adjust frame timing for odd frames halfway through the frame time
    if (millis() - lastDisplayTime >= (1000 / DISPLAY_REFRESH_RATE) / 2) {
        if (oddFrame && !oddFrameAdjusted) {
            tft.startWrite();

#ifdef TEARING_DEBUG
        // Set frame timing for even frames
        tft.writecommand(ST7789_FRCTR2);
        tft.writedata(var4);
        tft.writecommand(ST7789_PORCTRL);        
        tft.writedata(var5);
        tft.writedata(var6);
        tft.writedata(0x01); //enable separate porch control
        tft.writedata(0x33); // porch control in idle mode
        tft.writedata(0x33);// porch control in partial mode

#else

            tft.writecommand(ST7789_FRCTR2);
            tft.writedata(0x1a);
            tft.writecommand(ST7789_PORCTRL);               
            tft.writedata(0x40);
            tft.writedata(0x3f);
            tft.writedata(0x01); //enable separate port control
            tft.writedata(0x33); // portch control in idle mode
            tft.writedata(0x33);// porch control in partial mode
#endif // #ifdef TEARING_DEBUG

            tft.endWrite();
            oddFrameAdjusted = true;
        }
    }

    // Other non-blocking tasks can be performed here
}
