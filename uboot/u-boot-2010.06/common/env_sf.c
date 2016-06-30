/*
 * (C) Copyright 2000-2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2001 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Andreas Heppel <aheppel@sysgo.de>
 *
 * (C) Copyright 2008 Atmel Corporation
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <environment.h>
#include <malloc.h>
#include <spi_flash.h>

#ifndef CONFIG_ENV_SPI_BUS
# define CONFIG_ENV_SPI_BUS	0
#endif
#ifndef CONFIG_ENV_SPI_CS
# define CONFIG_ENV_SPI_CS		0
#endif
#ifndef CONFIG_ENV_SPI_MAX_HZ
# define CONFIG_ENV_SPI_MAX_HZ	1000000
#endif
#ifndef CONFIG_ENV_SPI_MODE
# define CONFIG_ENV_SPI_MODE	SPI_MODE_3
#endif

//int readfailsafecount();

DECLARE_GLOBAL_DATA_PTR;

/* references to names in env_common.c */
extern uchar default_environment[];

char * env_name_spec = "SPI Flash";
env_t *env_ptr;
char sysconfig[512];
static struct spi_flash *env_flash;

uchar env_get_char_spec(int index)
{
	return *((uchar *)(gd->env_addr + index));
}

int readfailsafecount()
{
	int ret;
        int i=0;
        env_flash = spi_flash_probe(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS,
                        CONFIG_ENV_SPI_MAX_HZ, CONFIG_ENV_SPI_MODE);
        if (!env_flash)
                goto err_probe;
//        printf("The CONFIG_ENV_OFFSET=%x,and CONFIG_ENV_SIZE=%x\n",CONFIG_ENV_OFFSET,CONFIG_ENV_SIZE);
        //Added by sanju test the image
        ret=spi_flash_read(env_flash,0x860000,30,sysconfig);
        if(ret==0)
        {

                printf("Boot Flag Reading Successfull\n");
       //         for(i=0;i<=20;i++)
       //                 printf("%c",sysconfig[i]);
        }
        else
        {
                printf("Read Failed\n");
        }
        if(sysconfig[0]!='V')
        {
                printf("Seting Default Boot parameters\n");
                sysconfig[0]='V';
                sysconfig[1]='V';
                sysconfig[2]='D';
                sysconfig[3]='N';
                sysconfig[4]='0';
        }
        else
        {
                printf("Current Value Fail Flag - %c\n",sysconfig[4]);
		sysconfig[4]=sysconfig[4]+1;
		if(!(sysconfig[4]>='0' && sysconfig[4]<='9'))
        	{
        		//Your failcount reached MAXimum go to secondary now
			return 1;
        	}
	}
        spi_flash_erase(env_flash,0x860000,0x10000);
        ret = spi_flash_write(env_flash, 0x860000,20, sysconfig);
        if(ret==0)
        {

                printf("Boot Mode Write Succesfull\n");
                //for(i=0;i<=20;i++)
                //        printf("%c\n",sysconfig[i]);
        }
        else
        {
                printf("Boot mode Write Failed\n");
        }
	return 0;

err_probe:
	return -1;


}



int saveenv(void)
{
	int ret;

	if (!env_flash) {
		puts("Environment SPI flash not initialized\n");
		return 1;
	}
//	env_ptr->crc= crc32(0, env_ptr->data, ENV_SIZE);
//	printf("Saving With The Crc now: Next time onwards no checksum error\n");
	puts("Writing to SPI flash...");
	ret = spi_flash_write(env_flash, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE, env_ptr);
	if (ret)
	{
		goto done;
	}
	ret = 0;
	puts("done\n");

 done:
	return ret;
}

void env_relocate_spec(void)
{
	int ret;
	int i=0;
	env_flash = spi_flash_probe(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS,
			CONFIG_ENV_SPI_MAX_HZ, CONFIG_ENV_SPI_MODE);
	if (!env_flash)
		goto err_probe;
	//printf("The CONFIG_ENV_OFFSET=%x,and CONFIG_ENV_SIZE=%x\n",CONFIG_ENV_OFFSET,CONFIG_ENV_SIZE);
//	readfailsafecount();
	ret = spi_flash_read(env_flash, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE, env_ptr);
	if (ret)
		goto err_read;
	//printf("This is ENV Size %x :sanju\n",ENV_SIZE);
	if (crc32(0, env_ptr->data, ENV_SIZE) != env_ptr->crc)
		goto err_crc;

	gd->env_valid = 1;

	return;

err_read:
	spi_flash_free(env_flash);
	env_flash = NULL;
err_probe:
err_crc:
	puts("*** Warning - bad CRC, using default environment always\n\n");

	set_default_env();
}

int env_init(void)
{
	/* SPI flash isn't usable before relocation */
	gd->env_addr = (ulong)&default_environment[0];
	gd->env_valid = 1;

	return 0;
}
