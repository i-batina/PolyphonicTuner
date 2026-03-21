#ifndef UI_H
#define UI_H

enum UIState { SCREEN_HOME, SCREEN_SELECT_TUNING, SCREEN_SETTINGS, SCREEN_TUNING };

class UI {
 public:
  void initUI();
  void handleLeft();
  void handleRight();
  void handleSelect();
  void handleBack();
  void renderCurrentScreen();

  // For Reading Inputs
  const char* getSelectedNote();
  const char* getSelectedType();
  int         getSelectedNoteIndex();
  int         getSelectedTypeIndex();

  bool isTuningActive();
  void stopTuning();

  // Called by Processing when a stable pitch is detected, to update the tuning screen
  static void setTuningDisplay(
      const char* current, const char* target, const char* strName, bool isSharp);

 private:
  static inline bool tuningActive   = false;  // Boolean to control if tuning should be happening
  static inline int  selectedIndex  = 0;
  static inline bool isEditingField = false;  // For Tuning Select Screen

  static inline const char* noteOptions[] = {"E", "Eb", "D", "Db", "C", "B", "Bb", "A"};
  static inline int         NOTE_COUNT    = sizeof(noteOptions) / sizeof(noteOptions[0]);

  static inline const char* typeOptions[] = {"Standard", "Drop"};
  static inline int         TYPE_COUNT    = sizeof(typeOptions) / sizeof(typeOptions[0]);

  // static const bool sharp; //Boolean to control if note should display as tuning up or down

  static inline int selectedNoteIndex = 0;  // For Tuning Select Screen
  static inline int selectedTypeIndex = 0;  // For Tuning Select Screen

  // Live data shown on the tuning screen
  static inline char _currentNote[16]   = "---";
  static inline char _targetNote[16]    = "---";
  static inline char _currentString[16] = "---";
  static inline bool _sharp             = false;
};

#endif  // UI_H