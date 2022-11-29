/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

/**
README 

Build steps: 
  > export PICO_SDK_PATH="/Users/andrew/pico/pico-sdk" 
  > cd /Users/andrew/pico/pico-examples/build 
  > cmake .. 
  > cd /Users/andrew/pico/pico-examples/build/usb/host/host_cdc_msc_hid
  > make 

Then: 
  * BootSel the Pico
  * Copy uf2 file from /Users/andrew/pico/pico-examples/build/usb/host/host_cdc_msc_hid

**/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>

#include "bsp/board.h"
#include "tusb.h" 

//--------------------------------------------------------------------+
//--                      START SECTION DVI                         --+
//--------------------------------------------------------------------+
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/vreg.h"
#include "pico/sem.h"

#include "dvi.h"
#include "dvi_serialiser.h"
#include "/Users/andrew/pico/PicoDVI/software/include/common_dvi_pin_configs.h"
#include "sprite.h"

void mal_init(void); 

// TMDS bit clock 252 MHz
// DVDD 1.2V (1.1V seems ok too)
#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240
#define VREG_VSEL VREG_VOLTAGE_1_20
#define DVI_TIMING dvi_timing_640x480p_60hz

#define LED_PIN 16

struct dvi_inst dvi0;
uint16_t framebuf[FRAME_WIDTH * FRAME_HEIGHT];

void core1_main() {
	dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
	dvi_start(&dvi0);
	dvi_scanbuf_main_16bpp(&dvi0);
	__builtin_unreachable();
}

void core1_scanline_callback() {
	// Discard any scanline pointers passed back
	uint16_t *bufptr;
	while (queue_try_remove_u32(&dvi0.q_colour_free, &bufptr))
		;
	// // Note first two scanlines are pushed before DVI start
	static uint scanline = 2;
	bufptr = &framebuf[FRAME_WIDTH * scanline];
	queue_add_blocking_u32(&dvi0.q_colour_valid, &bufptr);
	scanline = (scanline + 1) % FRAME_HEIGHT;
}

/*
  0xffff = white
  0x0fff = cyan
  0xf00f = red
*/
int drawColor = 0xf00f; 

void point(int x, int y) { 
  if((x >= 0) && (x < FRAME_WIDTH) && (y >= 0) && (y < FRAME_HEIGHT)){ 
    framebuf[y * FRAME_WIDTH + x] = drawColor;
  }  
}

/**
x1 must be less than or equal to x2
y1 must be less than or equal to y2
either x1,x2 or y1,y2 must be equal to each other
**/
void rect_line(int x1, int y1, int x2, int y2){ 
  if(x1 == x2){ 
    for(int i = y1; i <= y2; i++)
    { 
      point(x1, i);
    }
  } else if(y1 == y2) { 
    for(int i = x1; i <= x2; i++)
    { 
      point(i, y1);
    }
  }
}

void rect(int x, int y, int xw, int yw){ 
  rect_line(x, y, x+xw, y); 
  rect_line(x, y, x, y+yw); 
  rect_line(x+xw, y, x+xw, y+yw); 
  rect_line(x, y+yw, x+xw, y+yw); 
}



void lineLow(int x0, int y0, int x1, int y1) { 
    int dx = x1 - x0; 
    int dy = y1 - y0; 
    int yi = 1; 
    if( dy < 0 ) { 
        yi = -1; 
        dy = -dy; 
    }    
    int D = (2 * dy) - dx; 
    int y = y0; 

    for(int x=x0; x <=x1; x++){ 
        point(x, y); 
        if( D > 0 ) { 
            y = y + yi;
            D = D + (2 * (dy - dx));
        } else {
            D = D + 2*dy; 
        }
    }
}        

void lineHigh(int x0, int y0, int x1, int y1) { 
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xi = 1;
    if( dx < 0 ) { 
        xi = -1;
        dx = -dx;
    }    
    int D = (2 * dx) - dy;
    int x = x0;

    for(int y = y0; y <= y1; y++) {
        point(x, y); 
        if( D > 0 ) {
            x = x + xi; 
            D = D + (2 * (dx - dy)); 
        } else {
            D = D + 2*dx;
        }
    }    
}        

/**
no restrictions on location of points
**/
void line(int x0, int y0, int x1, int y1){ 
    if( abs(y1 - y0) < abs(x1 - x0) ) { 
        if( x0 > x1 ) { 
            lineLow(x1, y1, x0, y0);
        } else {
            lineLow(x0, y0, x1, y1);
        }    
    } else { 
        if( y0 > y1 ) {
            lineHigh(x1, y1, x0, y0);
        } else {
            lineHigh(x0, y0, x1, y1);
        }
    }
}

/**
TODO: 
  - create point array - DONE
  - create line array - DONE
  - create torus made from lines and points - DONE
  - draw torus (direct projection) - DONE
  - replace direct with perspective projection - DONE
    - add matrix function ( applies matrix to points )
    - use perspective matrix
  - add rotation matrix - DONE
  
  - back-face culling
    - add WORLD_FACES - DONE
    - add addFace() - DONE
    - modify createScene to create faces instead of lines - DONE
    - modify drawscene to draw faces instead of lines - DONE
    - test (still wire-frame) - DONE!
    - add function ccw - DONE
    - modify drawscene to select faces which are forward facing - DONE
    - test (back-face-cull) - DONE!!!
**/

#define PI 3.14159265358979

#define MAX_WORLD_POINT 100 
float worldPoint[MAX_WORLD_POINT][3]; 
int worldPoints = 0; //number of points

/**
#define MAX_WORLD_LINE 200 
int worldLine[MAX_WORLD_LINE][2]; 
int worldLines = 0; //number of lines 
**/

#define MAX_WORLD_FACE 100 
int worldFace[MAX_WORLD_FACE][4]; 
int worldFaces = 0; //number of faces 

int frame = 0; 

int addPoint(float x, float y, float z){ 
  int p = worldPoints; 
  worldPoints++; 
  worldPoint[p][0] = x; 
  worldPoint[p][1] = y; 
  worldPoint[p][2] = z; 
  return p; 
}

/*
void addLine(int a, int b){ 
  int l = worldLines; 
  worldLines++;
  worldLine[l][0] = a; 
  worldLine[l][1] = b; 
}
*/

void addFace(int a, int b, int c, int d){ 
  int f = worldFaces; 
  worldFaces++;
  worldFace[f][0] = a; 
  worldFace[f][1] = b; 
  worldFace[f][2] = c; 
  worldFace[f][3] = d; 
}

//https://en.wikipedia.org/wiki/Transformation_matrix

void applyPerspective(){ 
  for(int i = 0; i < worldPoints; i++){ 
    float x = worldPoint[i][0]; 
    float y = worldPoint[i][1]; 
    float z = worldPoint[i][2]; 
    
    float x1 = x / (z + 2); 
    float y1 = y / (z + 2); 
    
    worldPoint[i][0] = x1; 
    worldPoint[i][1] = y1; 
  }
}

void applyRotation(){ 
  for(int i = 0; i < worldPoints; i++){ 
    //float x = worldPoint[i][0]; 
    float y = worldPoint[i][1]; 
    float z = worldPoint[i][2]; 
    
    float t = ( (2 * PI) * (frame % 100) ) / 100 ;   
    
    float y1 = y * cos(t) - z * sin(t); 
    float z1 = y * sin(t) + z * cos(t); 
    
    worldPoint[i][1] = y1; 
    worldPoint[i][2] = z1; 
  }
}

/**
Note: for best results, make sure that the scene spans approx -1 to +1 in all directions.
**/

void createScene(){ 

  float r1 = 1; 
  float r2 = 0.25; 
  
  //re-initialize the scene: 
  worldPoints = 0; 
  worldFaces = 0; 
  
  //create points  
  for(int i = 0; i < 10; i++){ 
    for(int j = 0; j < 10; j++){ 
      
      //calculate angles: 
      float a = (PI * 2 * i) / 10.0; 
      float b = (PI * 2 * j) / 10.0;  

      addPoint( (r1 + r2 * cos(a) ) * cos(b), 
                (r1 + r2 * cos(a) ) * sin(b), 
                r2 * sin(a) );  

    }
  }
  
  applyRotation(); 
  applyPerspective(); 
  
  
/**  
  //create lines
  for(int i = 0; i < 10; i++){ 
    for(int j = 0; j < 10; j++){
      addLine((i * 10 + j), ((i + 1) % 10) * 10 + j);
      addLine((i * 10 + j), i * 10 + ((j + 1) % 10));
    }
  }
**/

  //create faces
  for(int i = 0; i < 10; i++){ 
    for(int j = 0; j < 10; j++){
      addFace((i * 10 + j), 
              ((i + 1) % 10) * 10 + j, 
              ((i + 1) % 10) * 10 + ((j + 1) % 10), 
              i * 10 + ((j + 1) % 10));
    }
  }
    
}

float ccw(int ai, int bi, int ci) {
   float ax = worldPoint[ai][0]; 
   float ay = worldPoint[ai][1]; 
   float bx = worldPoint[bi][0]; 
   float by = worldPoint[bi][1]; 
   float cx = worldPoint[ci][0]; 
   float cy = worldPoint[ci][1]; 
    
   return (bx - ax) * (cy - ay) - (cx - ax) * (by - ay);
} 

void drawScene(){ 

/**
  for(int i = 0; i < worldLines; i++){ 
    line( (FRAME_WIDTH / 2 ) + worldPoint[worldLine[i][0]][0] * 100,
          (FRAME_HEIGHT / 2 ) + worldPoint[worldLine[i][0]][1] * 100,
          (FRAME_WIDTH / 2 ) + worldPoint[worldLine[i][1]][0] * 100,
          (FRAME_HEIGHT / 2 ) + worldPoint[worldLine[i][1]][1] * 100); 
  }
**/  

  for(int i = 0; i < worldFaces; i++){ 
    for(int j = 0; j < 4; j++){ 
      int k = (j + 1) % 4;  
    
      float direction = ccw(worldFace[i][0],
                            worldFace[i][1],
                            worldFace[i][2]); 
                            
      if(direction > 0){   
    
        line( (FRAME_WIDTH / 2 ) + worldPoint[worldFace[i][j]][0] * 100,
              (FRAME_HEIGHT / 2 ) + worldPoint[worldFace[i][j]][1] * 100,
              (FRAME_WIDTH / 2 ) + worldPoint[worldFace[i][k]][0] * 100,
              (FRAME_HEIGHT / 2 ) + worldPoint[worldFace[i][k]][1] * 100 );
      }       
    }
  }
   
}

 
void draw(){       

  createScene(); 
  drawScene(); 
  
/*
      line(50, 50, 0, 0); 
      line(50, 50, 0, 20); 
      line(50, 50, 0, 50); 
      line(50, 50, 0, 70); 
      line(50, 50, 0, 100); 

      line(50, 50, 100, 0); 
      line(50, 50, 100, 20); 
      line(50, 50, 100, 50); 
      line(50, 50, 100, 70); 
      line(50, 50, 100, 100); 

      line(50, 50, 20, 0); 
      line(50, 50, 50, 0); 
      line(50, 50, 70, 0); 

      line(50, 50, 20, 100); 
      line(50, 50, 50, 100); 
      line(50, 50, 70, 100); 
*/
}

void setColor(int color){  
  drawColor = color; 
}

int mainDVI() {

	vreg_set_voltage(VREG_VSEL);
	sleep_ms(10);
#ifdef RUN_FROM_CRYSTAL
	set_sys_clock_khz(12000, true);
#else
	// Run system at TMDS bit clock
	set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);
#endif

	setup_default_uart();

	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	printf("Configuring DVI\n");

	dvi0.timing = &DVI_TIMING;
	dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
	dvi0.scanline_callback = core1_scanline_callback;
	dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

	// Once we've given core 1 the framebuffer, it will just keep on displaying
	// it without any intervention from core 0
	sprite_fill16(framebuf, 0x0000, FRAME_WIDTH * FRAME_HEIGHT);
	uint16_t *bufptr = framebuf;
	queue_add_blocking_u32(&dvi0.q_colour_valid, &bufptr);
	bufptr += FRAME_WIDTH;
	queue_add_blocking_u32(&dvi0.q_colour_valid, &bufptr);

	printf("Core 1 start\n");
	multicore_launch_core1(core1_main);

	printf("Start rendering\n");

    //int xpos = 0; 
    //int ypos = 0; 
    //int width = 100; 
    
    return(0);
}

int mainDVI_task(){ 
    
	  //sprite_fill16(framebuf, 0xffff, FRAME_WIDTH * FRAME_HEIGHT);
	  frame = frame + 1; 
    
      setColor(0xffff);
      draw();
      
	  //wait 40 ms
      //sleep_us(79690);   //79685 UP  79700 DOWN
      sleep_us(39845);
      	
      setColor(0x0000);
  	  draw();

  return(0);
}



//--------------------------------------------------------------------+
//--                       END SECTION DVI                          --+
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+
void led_blinking_task(void);

extern void cdc_task(void);
extern void hid_app_task(void);

/*------------- MAIN -------------*/
int main(void)  
{
  mainDVI();

  board_init();

  printf("TinyUSB Host CDC MSC HID Example\r\n");

  tusb_init(); 
  
  mal_init(); 

  while (1)
  {
    //disable for now
    //mainDVI_task();
  
    // tinyusb host task
    tuh_task();
    //led_blinking_task();  
    //board_led_write(true);

#if CFG_TUH_CDC
    cdc_task();
#endif

#if CFG_TUH_HID
    hid_app_task(); 
#endif
  }

  return 0;
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
#if CFG_TUH_CDC
CFG_TUSB_MEM_SECTION static char serial_in_buffer[64] = { 0 };

void tuh_mount_cb(uint8_t dev_addr)
{
  // application set-up
  printf("A device with address %d is mounted\r\n", dev_addr);
  
  //YES! We got here!
  //board_led_write(true);

  tuh_cdc_receive(dev_addr, serial_in_buffer, sizeof(serial_in_buffer), true); // schedule first transfer
}

void tuh_umount_cb(uint8_t dev_addr)
{
  // application tear-down
  printf("A device with address %d is unmounted \r\n", dev_addr);

  //DOESN'T go here!
  //board_led_write(true);

}

// invoked ISR context
void tuh_cdc_xfer_isr(uint8_t dev_addr, xfer_result_t event, cdc_pipeid_t pipe_id, uint32_t xferred_bytes)
{
  (void) event;
  (void) pipe_id;
  (void) xferred_bytes;

  printf(serial_in_buffer);
  tu_memclr(serial_in_buffer, sizeof(serial_in_buffer));

  tuh_cdc_receive(dev_addr, serial_in_buffer, sizeof(serial_in_buffer), true); // waiting for next data
}

void cdc_task(void) 
{

}

#endif

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+
//hello world!!!    !!! !!! !!!
//--------------------------------------------------------------------+
// Blinking Task
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  const uint32_t interval_ms = 1000;
  static uint32_t start_ms = 0;

  static bool led_state = false;

  // Blink every interval ms
  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}
