/*
    ZEsarUX  ZX Second-Emulator And Released for UniX
    Copyright (C) 2013 Cesar Hernandez Bano

    This file is part of ZEsarUX.

    ZEsarUX is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sms.h"
#include "vdp_9918a.h"
#include "cpu.h"
#include "debug.h"
//#include "ay38912.h"
#include "tape.h"
#include "screen.h"
#include "audio.h"
#include "sn76489an.h"
#include "joystick.h"


//Sega mater system emulation

//Memory Map - Development - SMS Power!
//https://www.smspower.org/Development/MemoryMap

//https://github.com/mikebenfield/euphrates/blob/master/euphrates/src/hardware/sms_io.rs

z80_byte *sms_vram_memory=NULL;





const char *sms_string_memory_type_rom="ROM";
const char *sms_string_memory_type_ram="RAM";
const char *sms_string_memory_type_empty="EMPTY";

z80_bit sms_cartridge_inserted={0};

char *sms_get_string_memory_type(int tipo)
{
    		
            

    switch (tipo) {

        case SMS_SLOT_MEMORY_TYPE_ROM:
            return (char *)sms_string_memory_type_rom;
        break;

        case SMS_SLOT_MEMORY_TYPE_RAM:
            return (char *)sms_string_memory_type_ram;
        break;

        default:
            return (char *)sms_string_memory_type_empty;
        break;

    }
}


//Retorna direccion de memoria donde esta mapeada la ram y su tipo
z80_byte *sms_return_segment_address(z80_int direccion,int *tipo)
{

/*
Region	Maps to
$0000-$bfff	Cartridge (ROM/RAM/etc)
$c000-$c3ff	System RAM
$c400-$ffff	System RAM (mirrored every 1KB)
*/

    //ROM
    if (direccion<=0xbfff) {
        *tipo=SMS_SLOT_MEMORY_TYPE_ROM;
        return &memoria_spectrum[direccion];
    }

    //RAM 1 KB
    else {
        *tipo=SMS_SLOT_MEMORY_TYPE_RAM;

        //old para sg1000
        //return &memoria_spectrum[0xc000 + (direccion & 1023)];

        //Para SMS asi:
        return &memoria_spectrum[0xc000 + (direccion & 8191)];
        /*
Master System/Mark III (assuming Sega mapper)
Region	Maps to
$0000-$03ff	ROM (unpaged)
$0400-$3fff	ROM mapper slot 0
$4000-$7fff	ROM mapper slot 1
$8000-$bfff	ROM/RAM mapper slot 2
$c000-$dfff	System RAM
$e000-$ffff	System RAM (mirror)
$fff8	3D glasses control
$fff9-$fffb	3D glasses control (mirrors)
$fffc	Cartridge RAM mapper control
$fffd	Mapper slot 0 control
$fffe	Mapper slot 1 control
$ffff	Mapper slot 2 control

        */
    }

    
    



}


void sms_init_memory_tables(void)
{




}


void sms_reset(void)
{

    //Resetear vram
    int i;

    for (i=0;i<16384;i++) sms_vram_memory[i]=0;

}

void sms_out_port_vdp_data(z80_byte value)
{
    vdp_9918a_out_vram_data(sms_vram_memory,value);
}


z80_byte sms_in_port_vdp_data(void)
{
    return vdp_9918a_in_vram_data(sms_vram_memory);
}



z80_byte sms_in_port_vdp_status(void)
{
    return vdp_9918a_in_vdp_status();
}

void sms_out_port_vdp_command_status(z80_byte value)
{
    vdp_9918a_out_command_status(value);
}





void sms_alloc_vram_memory(void)
{
    if (sms_vram_memory==NULL) {
        sms_vram_memory=malloc(16384);
        if (sms_vram_memory==NULL) cpu_panic("Cannot allocate memory for sms vram");
    }
}


z80_byte sms_read_vram_byte(z80_int address)
{
    //Siempre leer limitando a 16 kb
    return sms_vram_memory[address & 16383];
}



void sms_insert_rom_cartridge(char *filename)
{

	debug_printf(VERBOSE_INFO,"Inserting sms rom cartridge %s",filename);

    long tamanyo_archivo=get_file_size(filename);

    if (tamanyo_archivo>49152) {
        debug_printf(VERBOSE_ERR,"Cartridges bigger than 48K are not allowed");
        return;
    }

    FILE *ptr_cartridge;
    ptr_cartridge=fopen(filename,"rb");

    if (!ptr_cartridge) {
    debug_printf (VERBOSE_ERR,"Unable to open cartridge file %s",filename);
            return;
    }



    int leidos=fread(memoria_spectrum,1,tamanyo_archivo,ptr_cartridge);

    debug_printf(VERBOSE_INFO,"Loaded %d bytes of cartridge rom",leidos);
    

    fclose(ptr_cartridge);


    if (noautoload.v==0) {
            debug_printf (VERBOSE_INFO,"Reset cpu due to autoload");
            reset_cpu();
    }

    sms_cartridge_inserted.v=1;


}


void sms_empty_romcartridge_space(void)
{

//poner a 0
    int i;
    for (i=0;i<65536;i++) {
        memoria_spectrum[i]=0;
    }

    sms_cartridge_inserted.v=0;
}





//Refresco pantalla sin rainbow
void scr_refresca_pantalla_y_border_sms_no_rainbow(void)
{

 

    if (border_enabled.v && vdp_9918a_force_disable_layer_border.v==0) {
            //ver si hay que refrescar border
            if (modificado_border.v)
            {
                    vdp_9918a_refresca_border();
                    modificado_border.v=0;
            }

    }


    if (vdp_9918a_force_disable_layer_ula.v==0) {

        //Capa activada. Pero tiene reveal?

        if (vdp_9918a_reveal_layer_ula.v) {
            //En ese caso, poner fondo tramado
            int x,y;
            for (y=0;y<192;y++) {
                for (x=0;x<256;x++) {
                    int posx=x&1;
			        int posy=y&1;
                    int si_blanco_negro=posx ^ posy;
                    int color=si_blanco_negro*15;
                    scr_putpixel_zoom(x,y,  VDP_9918_INDEX_FIRST_COLOR+color);
                }
            }
        }
        else {
            vdp_9918a_render_ula_no_rainbow(sms_vram_memory);
        }
    }

    else {
        //En ese caso, poner fondo negro
        int x,y;
        for (y=0;y<192;y++) {
            for (x=0;x<256;x++) {
                scr_putpixel_zoom(x,y,  VDP_9918_INDEX_FIRST_COLOR+0);
            }
        }
    }



    if (vdp_9918a_force_disable_layer_sprites.v==0) {
        vdp_9918a_render_sprites_no_rainbow(sms_vram_memory);
    }
        
        


}


//Refresco pantalla con rainbow
void scr_refresca_pantalla_y_border_sms_rainbow(void)
{


	//aqui no tiene sentido (o si?) el modo simular video zx80/81 en spectrum
	int ancho,alto;

	ancho=get_total_ancho_rainbow();
	alto=get_total_alto_rainbow();

	int x,y,bit;

	//margenes de zona interior de pantalla. Para overlay menu
	int margenx_izq=screen_total_borde_izquierdo*border_enabled.v;
	int margenx_der=screen_total_borde_izquierdo*border_enabled.v+256;
	int margeny_arr=screen_borde_superior*border_enabled.v;
	int margeny_aba=screen_borde_superior*border_enabled.v+192;



	//para overlay menu tambien
	//int fila;
	//int columna;

	z80_int color_pixel;
	z80_int *puntero;

	puntero=rainbow_buffer;
	int dibujar;



	for (y=0;y<alto;y++) {


		//int altoborder=screen_borde_superior;

		
		for (x=0;x<ancho;x+=8) {
			dibujar=1;

			//Ver si esa zona esta ocupada por texto de menu u overlay

			if (y>=margeny_arr && y<margeny_aba && x>=margenx_izq && x<margenx_der) {
				if (!scr_ver_si_refrescar_por_menu_activo( (x-margenx_izq)/8, (y-margeny_arr)/8) )
					dibujar=0;
			}


			if (dibujar==1) {
					for (bit=0;bit<8;bit++) {
						color_pixel=*puntero++;
						scr_putpixel_zoom_rainbow(x+bit,y,color_pixel);
					}
			}
			else puntero+=8;

		}
		
	}




}


void scr_refresca_pantalla_y_border_sms(void)
{
    if (rainbow_enabled.v) {
        scr_refresca_pantalla_y_border_sms_rainbow();
    }
    else {
        scr_refresca_pantalla_y_border_sms_no_rainbow();
    }
}







//Almacenaje temporal de render de la linea actual
z80_int sms_scanline_buffer[512];


void screen_store_scanline_rainbow_sms_border_and_display(void) 
{

    screen_store_scanline_rainbow_vdp_9918a_border_and_display(sms_scanline_buffer,sms_vram_memory);


}




/*




            Mascara de puertos 0b11000001 = 193 = 0xC1


            


            //puerto DC =  1101 1100  - mask 0b11000001 (193 decimal) = 1100000000 -> joypad A

 Player 1                        A       B
            up down left right fire/space Z

Player 2 
             q   a    o    p     m       n 


             A B cont reset 

             Z X   C    R


             //Puerto DE = 1101 1110 - mask 0b11000001 = 1100 0000

*/


z80_byte sms_get_joypad_a(void) 
{


    //si estamos en el menu, no devolver tecla
    if (zxvision_key_not_sent_emulated_mach() ) return 255;

    z80_byte valor_joystick=255;

/*
joypad_a (value after mask 0b11000000 = 192)
JOYPAD2_DOWN:   = 0b10000000;
JOYPAD2_UP:     = 0b01000000;
JOYPAD1_B:      = 0b00100000;
JOYPAD1_A:      = 0b00010000;
JOYPAD1_RIGHT:  = 0b00001000;
JOYPAD1_LEFT:   = 0b00000100;
JOYPAD1_DOWN:   = 0b00000010;
JOYPAD1_UP:     = 0b00000001;
}
*/

//puerto_63486    db              255  ; 5    4    3    2    1     ;3
//puerto_61438    db              255  ; 6    7    8    9    0     ;4

			//z80_byte puerto_especial_joystick=0; //Fire Up Down Left Right

			if ((puerto_especial_joystick&1)) valor_joystick &=(255-8);
			if ((puerto_especial_joystick&2)) valor_joystick &=(255-4);
			if ((puerto_especial_joystick&4)) valor_joystick &=(255-2);
			if ((puerto_especial_joystick&8)) valor_joystick &=(255-1);

			if ((puerto_especial_joystick&16)) valor_joystick &=(255-16);

            //Espacio tambien vale como Fire/A
            //puerto_32766    db              255  ; B    N    M    Simb Space ;7
            if ((puerto_32766 & 1)==0) valor_joystick &=(255-16);

            //B = Tecla Z

            //puerto_65278   db    255  ; V    C    X    Z    Sh    ;0
            if ((puerto_65278 & 2)==0) valor_joystick &=(255-32);


            //Player 2. Q
            //puerto_64510    db              255  ; T    R    E    W    Q     ;2            
            if ((puerto_64510 & 1)==0) valor_joystick &=(255-64);




            //Player 2. A
            //puerto_65022   db    255  ; G    F    D    S    A     ;1
            if ((puerto_65022 & 1)==0) valor_joystick &=(255-128);





    return valor_joystick;
}




z80_byte sms_get_joypad_b(void) 
{

    //si estamos en el menu, no devolver tecla
    if (zxvision_key_not_sent_emulated_mach() ) return 255;


    z80_byte valor_joystick=255;

/*
value after mask port = 0b11000001 = 193
B_TH:           = 0b10000000;
A_TH:           = 0b01000000;
CONT:           = 0b00100000;
RESET:          = 0b00010000;
JOYPAD2_B:      = 0b00001000;
JOYPAD2_A:      = 0b00000100;
JOYPAD2_RIGHT:  = 0b00000010;
JOYPAD2_LEFT:   = 0b00000001;
}
*/

//puerto_63486    db              255  ; 5    4    3    2    1     ;3
//puerto_61438    db              255  ; 6    7    8    9    0     ;4

			//z80_byte puerto_especial_joystick=0; //Fire Up Down Left Right


            //Player 2. O
            //puerto_57342    db              255  ; Y    U    I    O    P     ;5         
            if ((puerto_57342 & 2)==0) valor_joystick &=(255-1);


            //Player 2. P
            //puerto_57342    db              255  ; Y    U    I    O    P     ;5         
            if ((puerto_57342 & 1)==0) valor_joystick &=(255-2);


            //Player 2. M
            //puerto_32766    db              255  ; B    N    M    Simb Space ;7
            if ((puerto_32766 & 4)==0) valor_joystick &=(255-4);


            //Player 2. N
            //puerto_32766    db              255  ; B    N    M    Simb Space ;7
            if ((puerto_32766 & 8)==0) valor_joystick &=(255-8);            

/*
             A B cont reset 

             Z X   C    R
*/

            //Player 2. Reset (R)
            //puerto_64510    db              255  ; T    R    E    W    Q     ;2
            if ((puerto_64510 & 8)==0) valor_joystick &=(255-16);   


            //Player 2. Cont (C)
            //puerto_65278   db    255  ; V    C    X    Z    Sh    ;0
            if ((puerto_65278 & 8)==0) valor_joystick &=(255-32); 


            //A  (Z)
            //puerto_65278   db    255  ; V    C    X    Z    Sh    ;0
            if ((puerto_65278 & 2)==0) valor_joystick &=(255-64); 

            //B  (X)
            //puerto_65278   db    255  ; V    C    X    Z    Sh    ;0
            if ((puerto_65278 & 4)==0) valor_joystick &=(255-128);             


    return valor_joystick;
}