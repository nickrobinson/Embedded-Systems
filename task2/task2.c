
//i believe this is most of what we need but probably not all

#define SIZE_OF_DATA_ARRAY		7;
#define END_OF_DATA_ARRAY		6;
#define MEDIAN_OF_DATA_ARRAY	3;

void user_init(void) {

CONFIG_AN1_AS_ANALOG();

esos_RegisterTask(read);
esos_RegisterTask(filter);
}

ESOS_USER_TASK(read) {
	
	static int data[SIZE_OF_DATA_ARRAY];
	static int counter;
	static uint16 u16_temperature;
	
	ESOS_TASK_BEGIN();
	while(TRUE) {
		//read in a temperature from LM60
		ESOS_TASK_WAIT_ON_AVAILABLE_IN_COMM();
		ESOS_TASK_WAIT_ON_GET_UINT16(u16_temperature);
		ESOS_TASK_SIGNAL_AVAILABLE_IN_COMM();
		data[counter] = u16_temperature;
		ESOS_TASK_WAIT_TICKS(100);
		counter + 1;
		
		
		if(counter == END_OF_DATA_ARRAY){
			counter = 0;
			ESOS_SIGNAL_SEMAPHORE(sem_dataRead, 1);
		}
		
		ESOS_TASK_WAIT_SEMAPHORE(sem_dataSent, 1);
	}
	ESOS_TASK_END();
}

ESOS_USER_TASK(filter) {

	static int sortingDummy, j;
	static uint16 u16_median;
	
	ESOS_TASK_BEGIN();
	while(TRUE) {
		ESOS_TASK_WAIT_SEMAPHORE(sem_dataRead, 1);
		
		//sort temperature data array
		for(int i=0; i<7; i+1) {
			sortingDummy = data[i];
			j = i-1;
			while(sortingDummy < data[j] && j >= 0)
			{
				data[j+1] = data[j];
				j = j-1;
			}
			data[j+1] = sortingDummy;
		}
		
		//choose median value from data array
		median = data[MEDIAN_OF_DATA_ARRAY];
		
		//export median to serial
		ESOS_TASK_WAIT_ON_AVAILABLE_OUT_COMM();
		ESOS_TASK_WAIT_ON_SEND_UINT16(median);
		ESOS_TASK_SIGNAL_AVAILABLE_OUT_COMM();
		ESOS_SIGNAL_SEMAPHORE(sem_dataSent, 1);
	}
	
	ESOS_TASK_END();
}