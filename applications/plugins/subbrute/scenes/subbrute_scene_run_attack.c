#include "../subbrute_i.h"
#include "subbrute_scene.h"
#include "../subbrute_custom_event.h"
#include "../views/subbrute_attack_view.h"

#define TAG "SubBruteSceneRunAttack"

static void subbrute_scene_run_attack_callback(SubBruteCustomEvent event, void* context) {
    furi_assert(context);

    SubBruteState* instance = (SubBruteState*)context;
    view_dispatcher_send_custom_event(instance->view_dispatcher, event);
}

static void
    subbrute_scene_run_attack_device_state_changed(void* context, SubBruteDeviceState state) {
    furi_assert(context);

    SubBruteState* instance = (SubBruteState*)context;

    if(state == SubBruteDeviceStateIDLE) {
        // Can't be IDLE on this step!
        view_dispatcher_send_custom_event(instance->view_dispatcher, SubBruteCustomEventTypeError);
    } else if(state == SubBruteDeviceStateFinished) {
        view_dispatcher_send_custom_event(
            instance->view_dispatcher, SubBruteCustomEventTypeTransmitFinished);
    }
}
void subbrute_scene_run_attack_on_exit(void* context) {
    furi_assert(context);
    SubBruteState* instance = (SubBruteState*)context;

    subbrute_worker_stop(instance->device);

    notification_message(instance->notifications, &sequence_blink_stop);
}

void subbrute_scene_run_attack_on_enter(void* context) {
    furi_assert(context);
    SubBruteState* instance = (SubBruteState*)context;
    SubBruteAttackView* view = instance->view_attack;

    instance->current_view = SubBruteViewAttack;
    subbrute_attack_view_set_callback(view, subbrute_scene_run_attack_callback, instance);
    view_dispatcher_switch_to_view(instance->view_dispatcher, instance->current_view);

    subbrute_device_set_callback(
        instance->device, subbrute_scene_run_attack_device_state_changed, instance);

    if(!subbrute_device_is_worker_running(instance->device)) {
        subbrute_worker_start(instance->device);
    }
}

bool subbrute_scene_run_attack_on_event(void* context, SceneManagerEvent event) {
    SubBruteState* instance = (SubBruteState*)context;
    SubBruteAttackView* view = instance->view_attack;

    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        subbrute_attack_view_set_current_step(view, subbrute_device_get_step(instance->device));

        if(event.event == SubBruteCustomEventTypeTransmitFinished) {
            notification_message(instance->notifications, &sequence_display_backlight_on);
            notification_message(instance->notifications, &sequence_single_vibro);

            scene_manager_next_scene(instance->scene_manager, SubBruteSceneSetupAttack);
        } else if(
            event.event == SubBruteCustomEventTypeTransmitNotStarted ||
            event.event == SubBruteCustomEventTypeBackPressed) {
            // Stop transmit
            scene_manager_search_and_switch_to_previous_scene(
                instance->scene_manager, SubBruteSceneSetupAttack);
        } else if(event.event == SubBruteCustomEventTypeError) {
            notification_message(instance->notifications, &sequence_error);
        } else if(event.event == SubBruteCustomEventTypeUpdateView) {
            //subbrute_attack_view_set_current_step(view, instance->device->key_index);
        }
        consumed = true;
    } else if(event.type == SceneManagerEventTypeTick) {
        subbrute_attack_view_set_current_step(view, subbrute_device_get_step(instance->device));

        consumed = true;
    }

    return consumed;
}