from rasa_sdk import Action, Tracker
from rasa_sdk.executor import CollectingDispatcher
import httpx
import os

API_KEY = os.getenv("OPENAI_API_KEY", "")
MODEL = os.getenv("OPENAI_MODEL", "gpt-4o")

SYSTEM_PROMPT = """Du bist ein freundlicher deutscher Sprachassistent.
Halte deine Antworten KURZ - maximal 2-3 Sätze.
Sei höflich und hilfsbereit."""


class ActionChatGptFallback(Action):
    def name(self):
        return "action_chatgpt_fallback"

    async def run(self, dispatcher: CollectingDispatcher, tracker: Tracker, domain):
        text = tracker.latest_message.get("text", "")

        if not API_KEY:
            dispatcher.utter_message(text="Entschuldigung, ich kann gerade nicht antworten.")
            return []

        try:
            async with httpx.AsyncClient(timeout=30.0) as client:
                resp = await client.post(
                    "https://api.openai.com/v1/chat/completions",
                    headers={"Authorization": f"Bearer {API_KEY}", "Content-Type": "application/json"},
                    json={
                        "model": MODEL,
                        "messages": [
                            {"role": "system", "content": SYSTEM_PROMPT},
                            {"role": "user", "content": text}
                        ],
                        "max_tokens": 500,
                        "temperature": 0.7
                    }
                )
                if resp.status_code == 200:
                    answer = resp.json()["choices"][0]["message"]["content"]
                    dispatcher.utter_message(text=" ".join(answer.split()))
                    return []
        except Exception:
            pass

        dispatcher.utter_message(text="Entschuldigung, ich konnte keine Antwort generieren.")
        return []
