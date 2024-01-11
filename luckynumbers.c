#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <luckynumbers_icons.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <furi_hal_random.h>


#define BOXTIME 2
#define VALLENGTH 7

typedef struct {
    FuriMessageQueue* input_queue;
    ViewPort* view_port;
    Gui* gui;
    FuriMutex** mutex;
    NotificationApp* notifications;
    bool pressed;
    int boxtimer;
    uint8_t finalValues[VALLENGTH];
    uint8_t randomValue[1];
} LuckyNumbers;

//Last Function to run to Free allocated memory
void state_free(LuckyNumbers* c) {
    gui_remove_view_port(c->gui, c->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(c->view_port);
    furi_message_queue_free(c->input_queue);
    furi_mutex_free(c->mutex);
    free(c);
}

//Receives a KEY PRESS and detexts if it is Long or short press, and puts it in a queue
static void input_callback(InputEvent* input_event, void* ctx) {
    LuckyNumbers* c = ctx;
    if(input_event->type == InputTypeShort || input_event->type == InputTypeLong) {
        furi_message_queue_put(c->input_queue, input_event, 0);
    }
}


//Render what will be showed in Flipper Screen
static void render_callback(Canvas* canvas, void* ctx) {
    LuckyNumbers* c = ctx;

    if(c->pressed){
    furi_check(furi_mutex_acquire(c->mutex, FuriWaitForever) == FuriStatusOk);
    //Calculate numbers
    for(int i=0; i<=4 ; i++){
        furi_hal_random_fill_buf(c->randomValue , 1);
        if (i==0){
            c->finalValues[i] = (c->randomValue[0]/4)+1;
            if (c->finalValues[i] > 50){
                c->finalValues[i] = c->finalValues[i] - 14;
            }
        }else{
            bool gotEqual = true;
            while(gotEqual){
                for(int j=0; j<=i;j++){
                    int curNum =(c->randomValue[0]/4)+1;
                    if (curNum > 50){
                        curNum = curNum - 14;
                    }
                    if(c->finalValues[j] == curNum){
                        furi_hal_random_fill_buf(c->randomValue , 1);
                        break;
                    }else{
                        gotEqual = false;
                    }
                }
            }
            c->finalValues[i] = (c->randomValue[0]/4)+1;
            if (c->finalValues[i] > 50){
                c->finalValues[i] = c->finalValues[i] - 14;
            }
        }
    }
    //Calculate Stars
    for(int i=5; i<=6 ; i++){
        furi_hal_random_fill_buf(c->randomValue , 1);
        if (i==0){
            c->finalValues[i] = (c->randomValue[0]/16)+1;
            if (c->finalValues[i] > 10){
                c->finalValues[i] = c->finalValues[i] - 6;
            }
        }else{
            bool gotEqual = true;
            while(gotEqual){
                for(int j=5; j<=i;j++){
                    int curStar =(c->randomValue[0]/16)+1;
                    if (curStar > 10){
                        curStar = curStar - 6;
                    }
                    if(c->finalValues[j] == curStar){
                        furi_hal_random_fill_buf(c->randomValue , 1);
                        break;
                    }else{
                        gotEqual = false;
                    }
                }
            }
            c->finalValues[i] = (c->randomValue[0]/16)+1;
            if (c->finalValues[i] > 10){
                c->finalValues[i] = c->finalValues[i] - 6;
            }
        }  
    }
    }
    c->pressed=false;
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);

    char strNums[32];
    snprintf(strNums, 32, "Numbers : %d %d %d %d %d", c->finalValues[0],c->finalValues[1],c->finalValues[2],c->finalValues[3],c->finalValues[4]);

    char strStars[32];
    snprintf(strStars, 32, "Stars : %d %d", c->finalValues[5],c->finalValues[6]);

    canvas_draw_str_aligned(canvas, 64, 24, AlignCenter, AlignCenter, strNums);
    canvas_draw_str_aligned(canvas, 64, 40, AlignCenter, AlignCenter, strStars);

    
    furi_mutex_release(c->mutex);
}

LuckyNumbers* state_init() {
    //Allocate Memory
    //Every malloc that is done should be freed later
    LuckyNumbers* c = malloc(sizeof(LuckyNumbers));
    c->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    c->view_port = view_port_alloc();
    c->gui = furi_record_open(RECORD_GUI);
    c->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    c->notifications = furi_record_open(RECORD_NOTIFICATION);
    //Atribute values to the vars
    c->boxtimer = 0;
    c->pressed = true;
    //View_PORT is the window showed on Flipper
    //Input Callback -> every input done call this function to read the call
    view_port_input_callback_set(c->view_port, input_callback, c);
    view_port_draw_callback_set(c->view_port, render_callback, c);
    gui_add_view_port(c->gui, c->view_port, GuiLayerFullscreen);

    //Inicializaes furirandom
    furi_hal_random_init();

    return c;
}

int32_t luckynumbersapp(void) {
    LuckyNumbers* c = state_init();

    InputEvent input;
    for(bool processing = true; processing;) {
        while(furi_message_queue_get(c->input_queue, &input, FuriWaitForever) == FuriStatusOk) {
            furi_check(furi_mutex_acquire(c->mutex, FuriWaitForever) == FuriStatusOk);

            if(input.type == InputTypeShort) {
                switch(input.key) {
                case InputKeyBack:
                    processing = false;
                    break;
                case InputKeyOk:
                    c->pressed = true;
                    c->boxtimer = BOXTIME;
                    break;
                default:
                    break;
                }
            }
            furi_mutex_release(c->mutex);
            if(!processing) {
                break;
            }
            view_port_update(c->view_port);
        }
    }
    state_free(c);
    return 0;
}
