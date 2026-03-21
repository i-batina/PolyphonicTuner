#ifndef UI_H
#define UI_H

enum UIState {
  SCREEN_HOME,
  SCREEN_SELECT_TUNING,
  SCREEN_SETTINGS,
  SCREEN_TUNING,
  SCREEN_PLUCK_PROMPT,
  SCREEN_ALL_TUNED
};

class UI {
 public:
  void initUI();
  void handleLeft();
  void handleRight();
  void handleSelect();
  void handleBack();
  void renderCurrentScreen();

  // --- Static accessors used by Processing ---
  static bool isTuningActive();
  static int  getSelectedTuningIndex();  // 0=Standard,1=Drop D,2=Open G,3=Open D

  // Called by Processing to drive UI state during the tuning workflow
  static void showPluckPrompt(
      const char* ordinal, const char* strLabel, const char* noteName, float targetHz);
  static void showTuningScreen();
  static void signalAllTuned();
  static void stopTuning();

  // Called by Processing when a stable pitch is detected (updates tuning screen data)
  static void setTuningDisplay(
      const char* current, const char* target, const char* strName, bool isSharp);

 private:
  static inline bool tuningActive       = false;
  static inline int  selectedIndex      = 0;
  static inline int  _selectedTuningIdx = 0;  // 0–3

  // Data for SCREEN_TUNING
  static inline char _currentNote[16]   = "---";
  static inline char _targetNote[16]    = "---";
  static inline char _currentString[16] = "---";
  static inline bool _sharp             = false;

  // Data for SCREEN_PLUCK_PROMPT
  static inline char  _pluckOrdinal[8]   = "1st";
  static inline char  _pluckStrLabel[16] = "Low E";
  static inline char  _pluckNoteName[8]  = "E";
  static inline float _pluckTargetHz     = 82.41f;
};

#endif  // UI_H