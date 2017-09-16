/*	for cc65, for NES
 *	simple platformer game
 *	doug fraker 2015
 *	feel free to reuse any code here
 */

#include "DEFINE.c"
#include "AudioEngine.c"

void main(void)
{
	All_Off();
	Draw_Title();

	joypad1 = 0xff; // fix a bug, reset is wiping joypad1old
	Load_Palette();
	Reset_Scroll();

	setupAudio();

	Wait_Vblank();
	All_On();
	while (1)
	{ // infinite loop
		while (Game_Mode == TITLE_MODE)
		{ // Title Screen
			while (NMI_flag == 0)
				; // wait till v-blank
			Rotate_Palette();
			Reset_Scroll();

			Get_Input();

			if (((joypad1old & START) == 0) && ((joypad1 & START) != 0))
			{
				NMI_flag = 0;
				while (NMI_flag == 0)
					; // wait till v-blank
				// init game mode
				All_Off();
				Game_Mode = RUN_GAME_MODE;
				Draw_Background();
				X1 = 0x80;
				Y1 = 0x70; // middle of screen
				Load_Palette();
				Reset_Scroll();

				Wait_Vblank();
				// was All_On(); changed to...
				PPU_CTRL = 0x91;
			}

			NMI_flag = 0;
		}

		while (Game_Mode == RUN_GAME_MODE)
		{ // Game Mode
			while (NMI_flag == 0)
				; // wait till v-blank

			//every_frame();	// moved this to the nmi code in reset.s for greater stability
			Get_Input();
			move_logic();
			audioUpdate();
			update_Sprites();

			NMI_flag = 0;
		}
	}
}

void update_Sprites(void)
{
	state4 = state << 2; // shift left 2 = multiply 4
	index4 = 0;
	if (direction == 0)
	{ // right
		for (index = 0; index < 4; ++index)
		{
			SPRITES[index4] = MetaSprite_Y[index] + Y1; // relative y + master y
			++index4;
			SPRITES[index4] = MetaSprite_Tile_R[index + state4]; // tile numbers
			++index4;
			SPRITES[index4] = MetaSprite_Attrib_R[index]; // attributes, all zero here
			++index4;
			SPRITES[index4] = MetaSprite_X[index] + X1; // relative x + master x
			++index4;
		}
	}
	else
	{ // left
		for (index = 0; index < 4; ++index)
		{
			SPRITES[index4] = MetaSprite_Y[index] + Y1; // relative y + master y
			++index4;
			SPRITES[index4] = MetaSprite_Tile_L[index + state4]; // tile numbers
			++index4;
			SPRITES[index4] = MetaSprite_Attrib_L[index]; // attributes, all zero here
			++index4;
			SPRITES[index4] = MetaSprite_X[index] + X1; // relative x + master x
			++index4;
		}
	}
}

void Collision_Down(void)
{
	if (NametableB == 0)
	{ // first collision map
		temp = C_MAP[collision_Index];
		collision += PLATFORM[temp];
	}
	else
	{ // second collision map
		temp = C_MAP2[collision_Index];
		collision += PLATFORM[temp];
	}
}
void move_logic(void)
{
	if ((joypad1 & (RIGHT | LEFT)) == 0)
	{ // no L or R
		walk_count = 0;
		// apply friction, if no L or R
		if (X_speed >= 0)
		{ // if positive, going right
			if (X_speed >= 4)
			{
				X_speed -= 4;
			}
			else
			{
				X_speed = 0;
			}
		}
		else
		{ // going left
			if (X_speed <= (-4))
			{
				X_speed += 4;
			}
			else
			{
				X_speed = 0;
			}
		}
	}

	if ((joypad1 & RIGHT) != 0)
	{
		++walk_count;
		direction = 0;
		if (X_speed >= 0)
		{ // going right
			X_speed += 2;
		}
		else
		{				  // going left
			X_speed += 8; // double friction
		}
	}
	if ((joypad1 & LEFT) != 0)
	{
		++walk_count;
		direction = 1;
		if (X_speed <= 0)
		{ // going left
			X_speed -= 2;
		}
		else
		{				  // going right
			X_speed -= 8; // double friction
		}
	}

	// collision check, platform

	// first check the bottom left corner of character
	// which nametable am I in?
	NametableB = Nametable;
	Scroll_Adjusted_X = (X1 + Horiz_scroll + 3); // left
	high_byte = Scroll_Adjusted_X >> 8;
	if (high_byte != 0)
	{					 // if H scroll + Sprite X > 255, then we should use
		++NametableB;	// the other nametable's collision map
		NametableB &= 1; // keep it 0 or 1
	}
	collision = 0;
	collisionBot = 0;
	// we want to find which metatile in the collision map this point is in...is it solid?
	collision_Index = (((char)Scroll_Adjusted_X >> 4) + ((Y1 + 16) & 0xf0)); //bottom left
	Collision_Down();
	collisionBot += collision;										  // if on platform, ++collision
	collision_Index = (((char)Scroll_Adjusted_X >> 4) + ((Y1)&0xf0)); //top left
	Collision_Down();
	NametableB = Nametable;
	Scroll_Adjusted_X = (X1 + Horiz_scroll + 13); // left
	high_byte = Scroll_Adjusted_X >> 8;
	if (high_byte != 0)
	{					 // if H scroll + Sprite X > 255, then we should use
		++NametableB;	// the other nametable's collision map
		NametableB &= 1; // keep it 0 or 1
	}
	collisionOld = collision;
	collision = 0;
	// we want to find which metatile in the collision map this point is in...is it solid?
	collision_Index = (((char)Scroll_Adjusted_X >> 4) + ((Y1 + 16) & 0xf0)); //bottom right
	Collision_Down();														 // if on platform, ++collision
	collisionBot += collision;
	collision = collisionOld+collision;
	collision_Index = (((char)Scroll_Adjusted_X >> 4) + ((Y1)&0xf0)); //top right
	Collision_Down();												  // if on platform, ++collision
	if (collision >= 50)
	{
		Y1 = 0x70;
		Horiz_scroll = 0x80;
		NametableB = Nametable;
		return;
	} // if on platform, ++collision
	if (Y_speed >= 0)
	{
		if ((Y1 & 0x0f) > 1) // only platform collide if nearly aligned to a metatile
			collisionBot = 0;

		if (collisionBot == 0)
		{
			Y_speed += 4; // gravity
		}
		else
		{
			Y_speed = 0; // collision = stop falling
			Y1 &= 0xf0;  // align to the metatile
		}
	}
	else
	{
		Y_speed += 4;
		if (collision < 5 && collision > 0)
		{
			Y_speed = 1;
			Y1 += 2;
		}
	}

	// Jump - we already figured if we are on a platform, only jump if on a platform
	if (collision > 0)
	{
		if (((joypad1 & A_BUTTON) != 0) && ((joypad1old & A_BUTTON) == 0))
		{
			Y_speed = -0x48; // 0xc8
			if(collision > 0)
			{
				audioBeep();
			}
		}
	}

	// max speeds
	if (X_speed >= 0)
	{ // going right
		if (X_speed > 0x20)
			X_speed = 0x20;
	}
	else
	{
		if (X_speed < (-0x20))
			X_speed = (-0x20); // 0xe0
	}

	if (Y_speed >= 0)
	{
		if (Y_speed > 0x20)
			Y_speed = 0x20;
	}
	if (X_speed != 0)
	{
		// move player
		Horiz_scroll_Old = Horiz_scroll;
		Horiz_scroll += (X_speed >> 4); // use the high nibble
		// which nametable am I in?
		NametableB = Nametable;
		Scroll_Adjusted_X = (X1 + Horiz_scroll + (X_speed < 0 ? 3 : 13)); // left
		high_byte = Scroll_Adjusted_X >> 8;
		if (high_byte != 0)
		{					 // if H scroll + Sprite X > 255, then we should use
			++NametableB;	// the other nametable's collision map
			NametableB &= 1; // keep it 0 or 1
		}
		// we want to find which metatile in the collision map this point is in...is it solid?
		collision = 0;																				  // if on platform, ++collision
		collision_Index = (((char)Scroll_Adjusted_X >> 4) + ((Y1 + (Y_speed <= 0 ? 15 : 16)) & 0xf0)); //top left if on ground / falling, bottom left if in air
		Collision_Down();
		collision_Index = (((char)Scroll_Adjusted_X >> 4) + ((Y1) & 0xf0)); //top left if on ground / falling, bottom left if in air
		Collision_Down();
		if (collision < 5 && collision > 0)
		{
			Horiz_scroll = Horiz_scroll_Old;
			X_speed = 0;
		}
		else
		{
			if (X_speed >= 0)
			{										 // going right
				if (Horiz_scroll_Old > Horiz_scroll) // if pass 0, switch nametables
					++Nametable;
			}
			else
			{ // going left
				if (Horiz_scroll_Old < Horiz_scroll)
					++Nametable; // if pass 0, switch nametables
			}
			Nametable &= 1; // keep it 1 or 0
		}
	}
	Y1 += (Y_speed >> 4); // use the high nibble

	if (walk_count > 0x1f) // walk_count forced 0-1f
		walk_count = 0;

	state = Walk_Moves[(walk_count >> 3)]; // if not jumping

	if (Y_speed < 0) // negative = jumping
		state = 3;
}

void Draw_Background(void)
{
	PPU_ADDRESS = 0x20; // address of nametable #0 = 0x2000
	PPU_ADDRESS = 0x00;
	UnRLE(N1); // uncompresses our data

	PPU_ADDRESS = 0x24; // address of nametable #1 = 0x2400
	PPU_ADDRESS = 0x00;
	UnRLE(N2); // uncompresses our data

	// load collision maps to RAM
	memcpy(C_MAP, C1, 240);
	memcpy(C_MAP2, C2, 240);
}
void All_Off(void)
{
	PPU_CTRL = 0;
	PPU_MASK = 0;
}

void All_On(void)
{
	PPU_CTRL = 0x90; // screen is on, NMI on
	PPU_MASK = 0x1e;
}

void Reset_Scroll(void)
{
	PPU_ADDRESS = 0;
	PPU_ADDRESS = 0;
	SCROLL = 0;
	SCROLL = 0;
}

void Load_Palette(void)
{
	PPU_ADDRESS = 0x3f;
	PPU_ADDRESS = 0x00;
	for (index = 0; index < sizeof(PALETTE); ++index)
	{
		PPU_DATA = PALETTE[index];
	}
}

const unsigned char Palette_Fade[] = { // for title screen
	0x24, 0x14, 0x04, 0x14};

void Rotate_Palette(void)
{ // for title screen
	PPU_ADDRESS = 0x3f;
	PPU_ADDRESS = 0x0b;
	PPU_DATA = Palette_Fade[(Frame_Count >> 2) & 0x03];
}

void Draw_Title(void)
{
	PPU_ADDRESS = 0x20; // address of nametable #0 = 0x2000
	PPU_ADDRESS = 0x00;
	UnRLE(Title); // uncompresses our data

	PPU_ADDRESS = 0x24; // draw the HUD on opposite nametable
	PPU_ADDRESS = 0x64;
	for (index = 0; index < sizeof(HUD); ++index)
	{
		PPU_DATA = HUD[index];
	}
	PPU_ADDRESS = 0x24;
	PPU_ADDRESS = 0x74;
	for (index = 0; index < sizeof(HUD); ++index)
	{
		PPU_DATA = HUD2[index];
	}
	// attribute table HUD
	PPU_ADDRESS = 0x27;
	PPU_ADDRESS = 0xc0;
	for (index = 0; index < 8; ++index)
	{
		PPU_DATA = 0xff;
	}
}