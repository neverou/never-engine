#include "event.h"




void EventBus::AddEventListener(EventListener listener, void* data) {
    ArrayAdd(&listeners, EventListenerEntry { listener, data });
}

void EventBus::PushEvent(const Event* event) {
    For (listeners) {
        EventListenerEntry listener = *it;
        listener.callback(event, listener.data);
    }
}


EventBus MakeEventBus() {
    EventBus eventBus;
    eventBus.listeners = MakeDynArray<EventListenerEntry>();
    return eventBus;
}

void DestroyEventBus(EventBus* eventBus) {
    FreeDynArray(&eventBus->listeners);
}