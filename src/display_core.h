/**
*@File display_core.h
*Handles basic creation, modification,
*and destruction of display objects
*/

#pragma once
#include "pebble.h"
#include "display_elements.h" 

/**
*initializes all display functionality
*@param themeResource the theme resource ID
*@param theme the theme index number
*@param fontIDs resource ids for all fonts
*@param fontMargins text margin height for adjusting font frames
*/
void display_create(int themeResource,int theme ,int fontIDs[],int fontMargins[]);

/**
*shuts down all display functionality
*/
void display_destroy();

/**
*Directly updates display text
*@param newText the updated text
*@param textID which text layer to set
*/
void update_display_text(char * newText, LayerID textID);

/**
*update one of the color values
*@param colorArray a 7 bit string with a valid color value
*@param colorID the ID of the color to update
*/
void update_color(char colorString[7], ColorID colorID);



/**
*Get the default bounds of a display layer
*@param layerID that object's ID
*@return the object's default bounds
*/
GRect get_default_frame(int layerID);

/**
*Get the current bounds of a display layer
*@param layerID that object's ID
*@return the image's bounds
*/
GRect get_bounds(int layerID);

/**
*Get the current frame of a display layer
*@param layerID that object's ID
*@return the image's frame
*/
GRect get_frame(int layerID);


/**
*set the bounds of a display layer
*@param bounds the new object bounds
*@param layerID the image's ID
*/
void set_bounds(GRect bounds, LayerID layerID);

/**
*set the frame of a display layer
*@param frame the new layer frame
*@param layerID the layer's ID
*/
void set_frame(GRect frame, LayerID layerID);

