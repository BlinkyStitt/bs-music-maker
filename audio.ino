
bool is_audio(String filename) {
  // TODO: there are more supported formats.
  return filename.endsWith(F(".MP3")) or filename.endsWith(F(".AAC")) or filename.endsWith(F(".MP4")) or
         filename.endsWith(F(".M4A")) or filename.endsWith(F(".WAV"));
}
