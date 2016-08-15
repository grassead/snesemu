#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "snes_apu.h"
#include "snes_apu_port.h"
#include "snes_apu_port_internal.h"
#include "snes_ram.h"

#define printf(...)

typedef enum _data_type{
	TRANSFER_INIT,
	TRANSFER_NEW,
	TRANSFER_END,
	TRANSFER_DATA,
	TRANSFER_INVALID,
	TRANSFER_NO_CHANGE,
} snes_apu_transfert_data_type_t;

typedef enum _apu_state{
	SNES_APU_STATE_STOPPED,
	SNES_APU_STATE_NOT_INIT,
	SNES_APU_STATE_INITED,
	SNES_APU_STATE_TRANSFERING,
	SNES_APU_STATE_EXECUTING,
} snes_apu_state;

typedef struct _snes_apu_tranfer_data{
	uint8_t port0;
	uint8_t port1;
	uint8_t port2;
	uint8_t port3;
}snes_apu_tranfer_data_t;

static int transfered = 0;
static int total_transfered = 0;

struct _snes_apu{
	snes_apu_port_t *port;
	snes_apu_state state;
	snes_ram_t *ram;
	pthread_t execution_thread;
	snes_apu_tranfer_data_t last_data;
	uint16_t current_transfert_addr;
};

static const char* snes_apu_tranfer_state_to_string(snes_apu_state state)
{
	switch(state) {
		case SNES_APU_STATE_STOPPED:
			return "SNES_APU_STATE_STOPPED";
		case SNES_APU_STATE_NOT_INIT:
			return "SNES_APU_STATE_NOT_INIT";
		case SNES_APU_STATE_INITED:
			return "SNES_APU_STATE_INITED";
		case SNES_APU_STATE_TRANSFERING:
			return "SNES_APU_STATE_TRANSFERING";
		case SNES_APU_STATE_EXECUTING:
			return "SNES_APU_STATE_EXECUTING";
		default:
			return "SNES_APU_STATE_UNKONWN";
	}
}

static void snes_apu_transfer_update_last_data(snes_apu_t *apu, snes_apu_tranfer_data_t *data)
{
	memcpy(&(apu->last_data), data, sizeof(snes_apu_tranfer_data_t));
}

static int snes_apu_transfer_is_same_data(snes_apu_t *apu, snes_apu_tranfer_data_t *data)
{
	if (memcmp(&(apu->last_data), data, sizeof(snes_apu_tranfer_data_t)) == 0) {
		return 1;
	} else if(data->port0 == apu->last_data.port0) {
		return 1;
	} else {
		return 0;
	}
}

static void snes_apu_transfer_data_dump(snes_apu_tranfer_data_t *data)
{
	printf("[APU] : port0 = 0x%02x",data->port0);
	printf(" || port1 = 0x%02x",data->port1);
	printf(" || port2 = 0x%02x",data->port2);
	printf(" || port3 = 0x%02x\n",data->port3);
}

static snes_apu_transfert_data_type_t snes_apu_transfer_get_type(snes_apu_t *apu, snes_apu_tranfer_data_t *data)
{
	snes_apu_transfert_data_type_t ret;
	uint8_t temp;

	if(snes_apu_transfer_is_same_data(apu, data)) {
		return TRANSFER_NO_CHANGE;
	}
	snes_apu_transfer_data_dump(data);

	if(data->port0 == 0xCC && apu->state < SNES_APU_STATE_INITED) {
		ret = TRANSFER_INIT;
		goto end;
	} else if(data->port0 == 0x00) {
		ret = TRANSFER_DATA;
		goto end;
	} else if(data->port0 == apu->last_data.port0 + 1) {
		ret = TRANSFER_DATA;
		goto end;
	}
	//Let see if it's a new transfer
	temp = apu->last_data.port0;
	temp += 4;
	if(temp == 0) {
		temp = 4;
	}
	if(data->port0 == temp) {
		if(data->port1 != 0) {
			ret = TRANSFER_NEW;
		} else {
			ret = TRANSFER_END;
		}
		goto end;
	}

	printf("[APU] : invalid transfer cookie = 0x%x (prev 0x%x)\n",data->port0,apu->last_data.port0);
	assert(0);

end:
	snes_apu_transfer_update_last_data(apu, data);
	return ret;
}


static void snes_apu_transfer_get_next_data(snes_apu_t *apu, snes_apu_tranfer_data_t *data)
{
	data->port0 = snes_apu_port_internal_read(apu->port, 0);
	data->port1 = snes_apu_port_internal_read(apu->port , 1);
	data->port2 = snes_apu_port_internal_read(apu->port, 2);
	data->port3 = snes_apu_port_internal_read(apu->port, 3);
}

static void snes_apu_set_state(snes_apu_t *apu, snes_apu_state state) {
	if (state != apu->state) {
		printf("[APU] : New state : %s\n",snes_apu_tranfer_state_to_string(state));
		apu->state = state;
	}
}
static uint16_t snes_apu_get_address(uint8_t low, uint8_t high)
{
	uint16_t addr;
	addr = high;
	addr = addr << 8;
	addr += low;
	return addr;
}

static int snes_apu_transfer_handle(snes_apu_t *apu)
{
	snes_apu_tranfer_data_t data;
	snes_apu_transfert_data_type_t data_type;

	snes_apu_transfer_get_next_data(apu, &data);
	data_type =	snes_apu_transfer_get_type(apu, &data);

	switch (data_type) {
		case TRANSFER_INIT :
		{
			snes_apu_set_state(apu, SNES_APU_STATE_INITED);
			apu->current_transfert_addr = snes_apu_get_address(data.port2,data.port3);
			printf("[APU] : Transfert adress is : 0x%04x\n",apu->current_transfert_addr);
			break;
		}
		case TRANSFER_DATA :
			printf("[APU] : New Data : 0x%02x to 0x%04x\n",data.port1, apu->current_transfert_addr);
			transfered++;
			snes_ram_write(apu->ram, apu->current_transfert_addr++, data.port1);
			break;
		case TRANSFER_NEW :
		{
			printf("[APU] : transfered for this block : 0x%x\n",transfered);
			printf("[APU] : End Addr for this block : 0x%04x\n",apu->current_transfert_addr - 1);
			total_transfered =+ transfered;
			transfered = 0;
			apu->current_transfert_addr = snes_apu_get_address(data.port2,data.port3);
			printf("[APU] : Transfert adress is : 0x%04x\n",apu->current_transfert_addr);
			break;
		}
		case TRANSFER_INVALID :
			assert(0);
			break;
		case TRANSFER_END:
			printf("[APU] : transfered for this block : 0x%x\n",transfered);
			printf("[APU] : End Addr for this block : 0x%04x\n",apu->current_transfert_addr - 1);
			snes_apu_set_state(apu, SNES_APU_STATE_EXECUTING);
			printf("[APU] : Execution adress is : 0x%02x%02x\n",data.port3,data.port2);
			total_transfered =+ transfered;
			transfered = 0;
			printf("[APU] : Total tansfered : 0x%x\n",total_transfered);
			do {
				if (apu->state == SNES_APU_STATE_STOPPED) {
					return 0;
				}
				usleep(10);
			}while (1);
			break;
		case TRANSFER_NO_CHANGE:
			return 0;
	}
	if (apu->state >= SNES_APU_STATE_INITED) {
		printf("[APU] : Writing cookie to port 0 0x%x\n",data.port0);
		snes_apu_port_internal_write(apu->port, 0, data.port0);
	}
	return 0;
}

static void* snes_apu_execute(void *data)
{
	snes_apu_t *apu = (snes_apu_t *)data;
	printf("[APU] : In APU execution thread\n");
	snes_apu_port_internal_write(apu->port, 0,0xAA);
	snes_apu_port_internal_write(apu->port, 1,0xBB);
	apu->state = SNES_APU_STATE_NOT_INIT;


	for(;;) {
		if (snes_apu_transfer_handle(apu) == 1) {
			break;
		}
		if (apu->state == SNES_APU_STATE_STOPPED) {
			break;
		}
		//usleep(10);
	}
	return NULL;
}

snes_apu_t *snes_apu_init()
{
	snes_apu_t *apu = malloc(sizeof(snes_apu_t));

	if(apu == NULL){
		goto error_alloc;
	}

	apu->port = snes_apu_port_init();
	if(apu->port == NULL) {
		goto error_port;
	}

	apu->ram = snes_ram_init(64 * 1024);
	if(apu->ram == NULL) {
		goto error_ram;
	}

	apu->state = SNES_APU_STATE_STOPPED;


	return apu;

error_ram:
	snes_apu_port_destroy(apu->port);
error_port:
	free(apu);
error_alloc:
	return NULL;
}

void snes_apu_destroy(snes_apu_t *apu)
{
	snes_apu_power_down(apu);
	snes_apu_port_destroy(apu->port);
	free(apu);
}

int snes_apu_power_up(snes_apu_t *apu)
{
	int ret = pthread_create (&apu->execution_thread, NULL,
							  snes_apu_execute, apu);
	return ret;
}

void snes_apu_power_down(snes_apu_t *apu)
{
	apu->state = SNES_APU_STATE_STOPPED;
	pthread_join(apu->execution_thread, NULL);
}


snes_apu_port_t *snes_apu_get_port(snes_apu_t *apu)
{
	return apu->port;
}
