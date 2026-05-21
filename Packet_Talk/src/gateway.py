import os
import struct
import ollama
import pyttsx3
import pygame
import re
from flask import Flask, request, Response
from faster_whisper import WhisperModel

app = Flask(__name__)

# --- Configuration ---
# Ensure this exactly matches the output of 'ollama list'
LLM_MODEL = "deepseek-r1" 

# --- Initialize ---
print(f"Loading Whisper...")
whisper_model = WhisperModel("base", device="cpu", compute_type="int8")
pygame.mixer.init()
print(f"Server ready. Using model: {LLM_MODEL}")

def safe_delete(filename):
    """Safely delete a file if it exists."""
    try:
        if os.path.exists(filename):
            os.remove(filename)
            print(f"[Cleanup] Deleted existing {filename}")
    except OSError as e:
        print(f"[Cleanup Warning] Could not delete {filename}: {e}")

def clean_deepseek_output(text):
    """Removes <think> tags and all content inside them."""
    cleaned = re.sub(r'<think>.*?</think>', '', text, flags=re.DOTALL)
    return cleaned.strip()

@app.route("/translate", methods=["POST"])
def handle_translation():
    # 0. PRE-CLEANUP
    pygame.mixer.music.stop()
    pygame.mixer.music.unload()
    safe_delete("input.wav")
    safe_delete("out.wav")

    # 1. Receive and Save Audio
    raw_pcm_data = request.get_data()
    if len(raw_pcm_data) < 2000: 
        return Response("Low data", status=400)
    
    with open("input.wav", "wb") as f:
        header = struct.pack('<4sI4s4sIHHIIHH4sI', b'RIFF', 36+len(raw_pcm_data), b'WAVE', b'fmt ', 16, 1, 1, 16000, 32000, 2, 16, b'data', len(raw_pcm_data))
        f.write(header + raw_pcm_data)

    # 2. Transcribe & Language Check
    segments, info = whisper_model.transcribe("input.wav", language=None)
    user_text = " ".join([seg.text for seg in segments]).strip()
    detected_lang = info.language
    
    print(f"\n[Whisper] Detected Language: {detected_lang} | Text: {user_text}")

    # THE GATEKEEPER: Only allow English ('en') or Tagalog ('tl', 'tgl')
    if detected_lang == 'en':
        source_lang, target_lang = 'English', 'Tagalog'
    elif detected_lang in ['tl', 'tgl']:
        source_lang, target_lang = 'Tagalog', 'English'
    else:
        print(f"[Rejected] Unsupported language detected: {detected_lang}")
        return Response("Unsupported language", status=200) # Returns to ESP32 without speaking
    
    if not user_text: 
        return Response("No speech", status=200)
    
    # 3. Translate
    prompt = f"Translate the following text from {source_lang} to {target_lang}. Output ONLY the translated text. Do not provide an explanation or reasoning. Text: '{user_text}'"
    
    print(f"[DeepSeek] -> Thinking...")
    response = ollama.generate(model=LLM_MODEL, prompt=prompt)
    
    # 4. Clean and Speak
    final_output = clean_deepseek_output(response['response'])
    print(f"[Final Result]: {final_output}")
    
    try:
        engine = pyttsx3.init()
        engine.setProperty('rate', 140)
        engine.save_to_file(final_output, "out.wav")
        engine.runAndWait() 
        
        if os.path.exists("out.wav"):
            pygame.mixer.music.load("out.wav")
            pygame.mixer.music.play()
    except Exception as e:
        print(f"[TTS Error]: {e}")
    
    return Response(final_output, mimetype="text/plain")

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5006, threaded=True)