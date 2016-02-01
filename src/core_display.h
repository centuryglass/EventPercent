/**
*@File core_display.h
*Handles basic creation, modification,
*and destruction of display objects
*/

#pragma once
#include "display_elements.h" 

/**
*initializes all display functionality
*/
void display_create();
/**
*shuts down all display functionality
*/
void display_destroy();

/**
*Directly updates display text
*@param newText the updated text
*@param textID which text layer to set
*/
void update_display_text(char * newText, TextLayerID textID);

/**
*update one of the color values
*@param colorArray a 7 bit string with a valid color value
*@param colorID the ID of the color to update
*/
void update_color(char colorString[7], ColorID colorID);

/**
*Sets the default color values before initialization
*@param defaultColors all default color values in colorID order
*/
void set_default_colors(GColor defaultColors[NUM_COLORS]);

//Defines valid layer types
typedef enum{
  TEXTLAYER_TYPE,
  BITMAP_LAYER_TYPE,
  GBITMAP_TYPE
}DisplayType;

/**
*Get the bounds of a display object
*@param type the type of object to get
*@param layerID that object's ID
*@return the object's bounds
*/
GRect get_bounds(DisplayType type,int layerID);

/**
*set the bounds of a display object
*@param bounds the new object bounds
*@param type the type of object to set
*@param layerID that object's ID
*/
void set_bounds(GRect bounds, DisplayType type,int layerID);

