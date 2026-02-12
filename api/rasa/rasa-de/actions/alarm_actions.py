from rasa_sdk import Action, Tracker
from rasa_sdk.executor import CollectingDispatcher
from rasa_sdk.events import SlotSet
import re
import random


def format_time(hour, minute):
    if minute == 0:
        return f"{hour} Uhr"
    return f"{hour} Uhr {minute}"


def parse_time(text):
    """Parse '7:30', '07:30', '7:30 Uhr' -> (hour, minute)"""
    cleaned = text.replace("Uhr", "").strip()
    match = re.match(r"(\d{1,2}):(\d{2})", cleaned)
    if match:
        return int(match.group(1)), int(match.group(2))
    return None, None


class ActionSetAlarm(Action):
    def name(self):
        return "action_set_alarm"

    def run(self, dispatcher: CollectingDispatcher, tracker: Tracker, domain):
        alarm = tracker.get_slot("alarm_time")

        if not alarm:
            dispatcher.utter_message(text="Auf welche Zeit soll ich den Wecker stellen?")
            return []

        hour, minute = parse_time(alarm)
        if hour is not None:
            time_str = format_time(hour, minute)
        else:
            time_str = alarm

        responses = [
            f"Wecker auf {time_str} gestellt!",
            f"Alles klar, ich wecke dich um {time_str}.",
            f"Dein Wecker ist auf {time_str} eingestellt.",
        ]
        dispatcher.utter_message(text=random.choice(responses))
        return [SlotSet("alarm_time", None)]


class ActionGetAlarm(Action):
    def name(self):
        return "action_get_alarm"

    def run(self, dispatcher: CollectingDispatcher, tracker: Tracker, domain):
        meta = tracker.latest_message.get("metadata", {})
        alarms = meta.get("alarms", [])
        count = len(alarms)

        if count == 0:
            dispatcher.utter_message(text="Du hast keine Wecker gestellt.")
            return []

        times = [format_time(a.get("hour", 0), a.get("minute", 0)) for a in alarms]

        if count == 1:
            msg = f"Du hast einen Wecker um {times[0]}."
        elif count == 2:
            msg = f"Du hast 2 Wecker: {times[0]} und {times[1]}."
        else:
            msg = f"Du hast {count} Wecker: {', '.join(times[:-1])} und {times[-1]}."

        dispatcher.utter_message(text=msg)
        return []
