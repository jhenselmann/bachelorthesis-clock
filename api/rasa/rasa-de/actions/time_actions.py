from rasa_sdk import Action
from rasa_sdk.events import SlotSet
import datetime


class ActionGetTime(Action):
    def name(self):
        return "action_get_time"

    def run(self, dispatcher, tracker, domain):
        cet = datetime.timezone(datetime.timedelta(hours=1))
        now = datetime.datetime.now(cet)

        if now.minute == 0:
            time_str = f"{now.hour} Uhr"
        else:
            time_str = f"{now.hour} Uhr {now.minute}"

        dispatcher.utter_message(text=f"Es ist {time_str}.")
        return [SlotSet("current_time", now.strftime("%H:%M"))]
