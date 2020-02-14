#include "stdint.h"
#include "stdbool.h"
#include "api_os.h"
#include "api_event.h"
#include "api_debug.h"
#include "app_config.h"

#include <api_gps.h>
#include "gps_parse.h"
#include "math.h"
#include "gps.h"

#include "api_mqtt.h"
#include "api_network.h"
#include "api_socket.h"
#include "mqtt_config.h"

#include "api_hal_gpio.h"
#include "api_hal_pm.h"

#include "api_info.h"

#define MAIN_TASK_STACK_SIZE    (1024 * 2)
#define MAIN_TASK_PRIORITY      0 
#define MAIN_TASK_NAME          "Main Task"

#define GPS_TASK_STACK_SIZE     (2048 * 2)
#define GPS_TASK_PRIORITY       1
#define GPS_TASK_NAME           "GPS Task"

#define MQTT_TASK_STACK_SIZE    (2048 * 2)
#define MQTT_TASK_PRIORITY      2
#define MQTT_TASK_NAME          "MQTT Task"

HANDLE handlerMain              = NULL;
HANDLE handlerGPS               = NULL;
HANDLE handlerMQTT              = NULL;
HANDLE handlerICCID             = NULL;
static HANDLE handlerStartMqtt  = NULL;

bool bGPSFlag                   = false;
bool isGpsOn                    = false;

GPIO_LEVEL gpiolGPS             = GPIO_LEVEL_LOW;
GPIO_LEVEL gpiolMQTT            = GPIO_LEVEL_LOW;
GPIO_config_t gpiocGPS          = { .mode = GPIO_MODE_OUTPUT, .pin = GPIO_GPS_LED , .defaultLevel = GPIO_LEVEL_LOW };
GPIO_config_t gpiocMQTT         = { .mode = GPIO_MODE_OUTPUT, .pin = GPIO_MQTT_LED, .defaultLevel = GPIO_LEVEL_LOW };
struct GPS_Location {
    double latitude;
    double longitude;
} gpsLocation;

typedef enum{
    MQTT_EVENT_CONNECTED = 0,
    MQTT_EVENT_DISCONNECTED ,
    MQTT_EVENT_MAX
}MQTT_Event_ID_t;

typedef struct {
    MQTT_Event_ID_t id;
    MQTT_Client_t* client;
}MQTT_Event_t;

typedef enum{
    MQTT_STATUS_DISCONNECTED = 0,
    MQTT_STATUS_CONNECTED       ,
    MQTT_STATUS_MAX
}MQTT_Status_t;

MQTT_Status_t mqttStatus = MQTT_STATUS_DISCONNECTED;

void BlinkLed(GPIO_config_t gpioConfig, int interval, GPIO_LEVEL afterblink)
{
    GPIO_LEVEL lv = GPIO_LEVEL_LOW;
    for(int i=0; i<interval; i+=50)
    {
        lv = lv == GPIO_LEVEL_HIGH ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
        GPIO_SetLevel(gpioConfig,lv);
        OS_Sleep(50);
    }
    GPIO_SetLevel(gpioConfig, afterblink);
}
//***********************MQTT Part***********************//
void OnMqttReceived(void* arg, const char* topic, uint32_t payloadLen)
{
    Trace(1,"#InD MQTT received publish data request, topic: %s, payload length:%d",topic,payloadLen);
}

void OnMqttReceiedData(void* arg, const uint8_t* data, uint16_t len, MQTT_Flags_t flags)
{
    Trace(1,"#InD MQTT recieved publish data,  length: %d, data: %s",len,data);
    if(flags == MQTT_FLAG_DATA_LAST)
        Trace(1,"#InD MQTT data is last frame");
}

void OnMqttSubscribed(void* arg, MQTT_Error_t err)
{
    if(err != MQTT_ERROR_NONE)
      Trace(1,"#InD MQTT subscribe fail, error code: %d",err);
    else
      Trace(1,"#InD MQTT subscribe success, topic: %s",(const char*)arg);
 }
void OnMqttConnection(MQTT_Client_t *client, void *arg, MQTT_Connection_Status_t status)
{
    Trace(1,"#InD MQTT connection status: %d",status);
    MQTT_Event_t* event = (MQTT_Event_t*)OS_Malloc(sizeof(MQTT_Event_t));
    if(!event)
    {
        Trace(1,"#InD MQTT no memory");
        return ;
    }
    if(status == MQTT_CONNECTION_ACCEPTED)
    {
        Trace(1,"#InD MQTT succeed connect to broker");
        //!!! DO NOT suscribe here(interrupt function), do MQTT suscribe in task, or it will not excute
        event->id = MQTT_EVENT_CONNECTED;
        event->client = client;
        OS_SendEvent(handlerMQTT,event,OS_TIME_OUT_WAIT_FOREVER,OS_EVENT_PRI_NORMAL);
    }
    else
    {
        event->id = MQTT_EVENT_DISCONNECTED;
        event->client = client;
        OS_SendEvent(handlerMQTT,event,OS_TIME_OUT_WAIT_FOREVER,OS_EVENT_PRI_NORMAL);
        Trace(1,"#InD MQTT connect to broker fail,error code: %d",status);
    }
    Trace(1,"#InD MQTT OnMqttConnection() end");
}

void StartTimerPublish(uint32_t interval,MQTT_Client_t* client);
void OnPublish(void* arg, MQTT_Error_t err)
{
    if(err == MQTT_ERROR_NONE)
        Trace(1,"#InD MQTT publish success");
    else
        Trace(1,"#InD MQTT publish error, error code: %d",err);
}

void OnTimerPublish(void* param)
{
    uint8_t buffer[30];
    MQTT_Error_t err;
    MQTT_Client_t* client = (MQTT_Client_t*)param;
    if(mqttStatus != MQTT_STATUS_CONNECTED)
    {
        GPIO_SetLevel(gpiocMQTT, GPIO_LEVEL_LOW);
        Trace(1,"#InD MQTT not connected to broker! can not publish");
        return;
    }
    Trace(1,"#InD MQTT OnTimerPublish");
    snprintf(buffer,sizeof(buffer),"%f,%f", gpsLocation.latitude, gpsLocation.longitude);

    err = MQTT_Publish(client,PUBLISH_TOPIC,buffer,strlen(buffer),1,2,0,OnPublish,NULL);
    //err = MQTT_ERROR_ABRT;
    if(err != MQTT_ERROR_NONE)
    {
        Trace(1,"#InD MQTT publish error, error code:%d location: %f %f",err, gpsLocation.latitude, gpsLocation.longitude);
        GPIO_SetLevel(gpiocMQTT, GPIO_LEVEL_LOW);
    }
    else
    {
        BlinkLed(gpiocMQTT, 500, GPIO_LEVEL_HIGH);
    }
    StartTimerPublish(PUBLISH_INTERVAL,client);
}

void StartTimerPublish(uint32_t interval,MQTT_Client_t* client)
{
    OS_StartCallbackTimer(handlerMain,interval,OnTimerPublish,(void*)client);
}
//***********************End of MQTT Part***********************//
void SetImei(uint8_t imei[])
{
    strncpy(deviceImei,imei,21);
    Trace(1,"#InD set imei to %s", deviceImei);
}
void EventDispatch(API_Event_t* pEvent)
{
    switch(pEvent->id)
    {
        case API_EVENT_ID_POWER_ON:
            break;
        
        case API_EVENT_ID_NO_SIMCARD:
            Trace(1,"#InD MQTT !!NO SIM CARD%d!!!!",pEvent->param1);
            break;
        
        case API_EVENT_ID_NETWORK_REGISTERED_HOME:
        case API_EVENT_ID_NETWORK_REGISTERED_ROAMING:
            Trace(1,"#InD MQTT network register success");
            Network_StartAttach();
            bGPSFlag = true;
            break;
        
        case API_EVENT_ID_SYSTEM_READY:
            Trace(1,"#InD MQTT system initialize complete");
            break;
        
        case API_EVENT_ID_NETWORK_ATTACHED:
            Trace(1,"#InD MQTT network attach success");
            Network_PDP_Context_t context = {
                .apn        ="cmnet",
                .userName   = ""    ,
                .userPasswd = ""
            };
            Network_StartActive(context);
            break;

        case API_EVENT_ID_NETWORK_ACTIVATED:
            Trace(1,"#InD MQTT network activate success.."); 
            OS_ReleaseSemaphore(handlerStartMqtt);
            break;
        
        case API_EVENT_ID_SOCKET_CONNECTED:
            Trace(1,"#InD MQTT socket connected");
            break;
        
        case API_EVENT_ID_SOCKET_CLOSED:
            Trace(1,"#InD MQTT socket closed");            
            break;

        case API_EVENT_ID_SIGNAL_QUALITY:
            Trace(1,"#InD CSQ:%d",pEvent->param1);
            break;

        case API_EVENT_ID_GPS_UART_RECEIVED:
            // Trace(1,"received GPS data,length:%d, data:%s,flag:%d",pEvent->param1,pEvent->pParam1,flag);
            GPS_Update(pEvent->pParam1,pEvent->param1);
            break;
        default:
            break;
    }
}

void EventDispatcherMQTT(MQTT_Event_t* pEvent)
{
    switch(pEvent->id)
    {
        case MQTT_EVENT_CONNECTED:
            mqttStatus = MQTT_STATUS_CONNECTED;
            Trace(1,"#InD MQTT connected, now subscribe topic: %s",SUBSCRIBE_TOPIC);
            MQTT_Error_t err;
            MQTT_SetInPubCallback(pEvent->client, OnMqttReceived, OnMqttReceiedData, NULL);
            err = MQTT_Subscribe(pEvent->client,SUBSCRIBE_TOPIC,2,OnMqttSubscribed,(void*)SUBSCRIBE_TOPIC);
            if(err != MQTT_ERROR_NONE)
                Trace(1,"#InD MQTT subscribe error, error code: %d",err);
            StartTimerPublish(PUBLISH_INTERVAL,pEvent->client);
            break;
        case MQTT_EVENT_DISCONNECTED:
            mqttStatus = MQTT_STATUS_DISCONNECTED;
            break;
        default:
            break;
    }
}

void TaskMQTT(void *pData)
{
    MQTT_Event_t* event=NULL;
    handlerStartMqtt = OS_CreateSemaphore(0);
    OS_WaitForSemaphore(handlerStartMqtt,OS_WAIT_FOREVER);
    OS_DeleteSemaphore(handlerStartMqtt);
    uint8_t imei[21];
    memset(imei,0,sizeof(imei));
    if(INFO_GetIMEI(imei))
    {
        Trace(1,"#InD imei: %s",imei);
        SetImei(imei);
    }
    else
    {
        Trace(1,"#InD Failed to get imei");
    }
    Trace(1,"#InD MQTT is starting ...");
    MQTT_Client_t* client = MQTT_ClientNew();
    MQTT_Connect_Info_t ci;
    MQTT_Error_t err;
    memset(&ci,0,sizeof(MQTT_Connect_Info_t));
    ci.client_id = CLIENT_ID;
    ci.client_user = CLIENT_USER;
    ci.client_pass = CLIENT_PASS;
    ci.keep_alive = 60;
    ci.clean_session = 1;
    ci.use_ssl = false;

    err = MQTT_Connect(client,BROKER_IP,BROKER_PORT,OnMqttConnection,NULL,&ci);
    if(err != MQTT_ERROR_NONE)
    {
        GPIO_SetLevel(gpiocMQTT,GPIO_LEVEL_LOW);
        Trace(1,"#InD MQTT connect fail, error code: %d",err);
    }
    
    while(1)
    {
        if(OS_WaitEvent(handlerMQTT, (void**)&event, OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatcherMQTT(event);
            OS_Free(event);
        }
    }
}

void TaskGPS(void *pData)
{
    GPS_Info_t* gpsInfo = Gps_GetInfo();
    uint8_t buffer[300];

    //wait for gprs register complete
    //The process of GPRS registration network may cause the power supply voltage of GPS to drop,
    //which resulting in GPS restart.
    while(!bGPSFlag)
    {
        Trace(1,"#InD wait for gprs register complete");
        OS_Sleep(2000);
    }

    //open GPS hardware(UART2 open either)
    GPS_Init();
    GPS_Open(NULL);
    GPIO_SetLevel(gpiocGPS,GPIO_LEVEL_HIGH);
    //wait for gps start up, or gps will not response command
    while(gpsInfo->rmc.latitude.value == 0)
        OS_Sleep(1000);
    

    // set gps nmea output interval
    for(uint8_t i = 0;i<5;++i)
    {
        bool ret = GPS_SetOutputInterval(10000);
        Trace(1,"#InD set gps ret: %d",ret);
        if(ret)
            break;
        OS_Sleep(1000);
    }

    if(!GPS_GetVersion(buffer,150))
        Trace(1,"#InD get gps firmware version fail");
    else
        Trace(1,"#InD gps firmware version: %s",buffer);

    if(!GPS_SetOutputInterval(1000))
        Trace(1,"#InD set nmea output interval fail");
    
    Trace(1,"#InD init ok");
    isGpsOn = true;

    while(1)
    {
        if(isGpsOn)
        { 
            GPIO_SetLevel(gpiocGPS,GPIO_LEVEL_HIGH);
            //show fix info
            uint8_t isFixed = gpsInfo->gsa[0].fix_type > gpsInfo->gsa[1].fix_type ?gpsInfo->gsa[0].fix_type:gpsInfo->gsa[1].fix_type;
            char* isFixedStr = "";            
            if(isFixed == 2)
                isFixedStr = "2D fix";
            else if(isFixed == 3)
            {
                if(gpsInfo->gga.fix_quality == 1)
                    isFixedStr = "3D fix";
                else if(gpsInfo->gga.fix_quality == 2)
                    isFixedStr = "3D/DGPS fix";
            }
            else
                isFixedStr = "no fix";

            //convert unit ddmm.mmmm to degree(°) 
            int temp = (int)(gpsInfo->rmc.latitude.value/gpsInfo->rmc.latitude.scale/100);
            double latitude = temp+(double)(gpsInfo->rmc.latitude.value - temp*gpsInfo->rmc.latitude.scale*100)/gpsInfo->rmc.latitude.scale/60.0;
            temp = (int)(gpsInfo->rmc.longitude.value/gpsInfo->rmc.longitude.scale/100);
            double longitude = temp+(double)(gpsInfo->rmc.longitude.value - temp*gpsInfo->rmc.longitude.scale*100)/gpsInfo->rmc.longitude.scale/60.0;

            //you can copy ` latitude,longitude ` to http://www.gpsspg.com/maps.htm check location on map

            //snprintf(buffer,sizeof(buffer),"#InD GPS fix mode:%d, BDS fix mode:%d, fix quality:%d, satellites tracked:%d, gps sates total:%d, is fixed:%s, coordinate:WGS84, Latitude:%f, Longitude:%f, unit:degree,altitude:%f",gpsInfo->gsa[0].fix_type, gpsInfo->gsa[1].fix_type,
            //                                                    gpsInfo->gga.fix_quality,gpsInfo->gga.satellites_tracked, gpsInfo->gsv[0].total_sats, isFixedStr, latitude,longitude,gpsInfo->gga.altitude);
            //show in tracer
            //Trace(2,buffer);
            gpsLocation.latitude = latitude;
            gpsLocation.longitude = longitude;
            //send to UART1
        }
        else
        {   
            GPIO_SetLevel(gpiocGPS,GPIO_LEVEL_LOW);
        }
        OS_Sleep(5000);
    }
}

void AppMainTask(VOID *pData)
{
    API_Event_t* event = NULL;

    PM_PowerEnable(POWER_TYPE_VPAD,true);
    GPIO_Init(gpiocGPS);
    GPIO_Init(gpiocMQTT);  

    handlerMQTT = OS_CreateTask(TaskMQTT,
        NULL, NULL, MQTT_TASK_STACK_SIZE, MQTT_TASK_PRIORITY, 0, 0, MQTT_TASK_NAME);

    handlerGPS = OS_CreateTask(TaskGPS,
        NULL, NULL, MQTT_TASK_STACK_SIZE, GPS_TASK_PRIORITY, 0, 0, GPS_TASK_NAME);
        
    while(1)
    {
        if(OS_WaitEvent(handlerMain, (void**)&event, OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatch(event);
            OS_Free(event->pParam1);
            OS_Free(event->pParam2);
            OS_Free(event);
        }
    }
}

void app_Main(void)
{
    handlerMain = OS_CreateTask(AppMainTask ,
        NULL, NULL, MAIN_TASK_STACK_SIZE, MAIN_TASK_PRIORITY, 0, 0, MAIN_TASK_NAME);
    OS_SetUserMainHandle(&handlerMain);
}