#include "ui.h"
#include "display.h"
#include <string.h>

static Display display;
static UIState currentState  = SCREEN_HOME;
static UIState previousState = SCREEN_HOME;

// Number of selectable items per screen
static int getOptionCountForState(UIState state) {
  switch (state) {
    case SCREEN_HOME:
      return 3;
    case SCREEN_SELECT_TUNING:
      return 4;  // 4 named tunings
    case SCREEN_SETTINGS:
      return 3;
    case SCREEN_TUNING:
    case SCREEN_PLUCK_PROMPT:
    case SCREEN_ALL_TUNED:
      return 0;
    default:
      return 1;
  }
}

static UIState getParentState(UIState state) {
  switch (state) {
    case SCREEN_SELECT_TUNING:
      return SCREEN_HOME;
    case SCREEN_TUNING:
      return SCREEN_SELECT_TUNING;
    case SCREEN_PLUCK_PROMPT:
      return SCREEN_SELECT_TUNING;
    case SCREEN_ALL_TUNED:
      return SCREEN_HOME;
    case SCREEN_SETTINGS:
      return SCREEN_HOME;
    default:
      return SCREEN_HOME;
  }
}

// --- Static method implementations ---

bool UI::isTuningActive() { return tuningActive; }

int UI::getSelectedTuningIndex() { return _selectedTuningIdx; }

void UI::showPluckPrompt(
    const char* ordinal, const char* strLabel, const char* noteName, float targetHz) {
  strncpy(_pluckOrdinal, ordinal, sizeof(_pluckOrdinal) - 1);
  _pluckOrdinal[sizeof(_pluckOrdinal) - 1] = '\0';
  strncpy(_pluckStrLabel, strLabel, sizeof(_pluckStrLabel) - 1);
  _pluckStrLabel[sizeof(_pluckStrLabel) - 1] = '\0';
  strncpy(_pluckNoteName, noteName, sizeof(_pluckNoteName) - 1);
  _pluckNoteName[sizeof(_pluckNoteName) - 1] = '\0';
  _pluckTargetHz                             = targetHz;
  currentState                               = SCREEN_PLUCK_PROMPT;
}

void UI::showTuningScreen() { currentState = SCREEN_TUNING; }

void UI::signalAllTuned() {
  tuningActive = false;
  currentState = SCREEN_ALL_TUNED;
}

void UI::stopTuning() {
  tuningActive  = false;
  currentState  = SCREEN_SELECT_TUNING;
  selectedIndex = 0;
}

void UI::setTuningDisplay(
    const char* current, const char* target, const char* strName, bool isSharp) {
  strncpy(_currentNote, current, sizeof(_currentNote) - 1);
  _currentNote[sizeof(_currentNote) - 1] = '\0';
  strncpy(_targetNote, target, sizeof(_targetNote) - 1);
  _targetNote[sizeof(_targetNote) - 1] = '\0';
  strncpy(_currentString, strName, sizeof(_currentString) - 1);
  _currentString[sizeof(_currentString) - 1] = '\0';
  _sharp                                     = isSharp;
}

// --- Instance method implementations ---

void UI::initUI() {
  currentState  = SCREEN_HOME;
  previousState = SCREEN_HOME;
  selectedIndex = 0;
}

void UI::handleLeft() {
  int count = getOptionCountForState(currentState);
  if (count == 0) return;
  selectedIndex--;
  if (selectedIndex < 0) selectedIndex = count - 1;
}

void UI::handleRight() {
  int count = getOptionCountForState(currentState);
  if (count == 0) return;
  selectedIndex++;
  if (selectedIndex >= count) selectedIndex = 0;
}

void UI::handleSelect() {
  switch (currentState) {
    case SCREEN_HOME:
      if (selectedIndex == 0)
        currentState = SCREEN_SELECT_TUNING;
      else if (selectedIndex == 1)
        currentState = SCREEN_SETTINGS;
      selectedIndex = 0;
      break;

    case SCREEN_SELECT_TUNING:
      // selectedIndex 0–3 IS the tuning choice; pressing Select starts tuning
      _selectedTuningIdx = selectedIndex;
      tuningActive       = true;
      selectedIndex      = 0;
      // Processing will call showPluckPrompt() for string 0 on its first loop
      currentState = SCREEN_PLUCK_PROMPT;
      break;

    case SCREEN_ALL_TUNED:
      tuningActive  = false;
      currentState  = SCREEN_HOME;
      selectedIndex = 0;
      break;

    case SCREEN_PLUCK_PROMPT:
    case SCREEN_TUNING:
    case SCREEN_SETTINGS:
      break;
  }
}

void UI::handleBack() {
  if (currentState == SCREEN_TUNING || currentState == SCREEN_PLUCK_PROMPT) {
    tuningActive  = false;
    currentState  = SCREEN_SELECT_TUNING;
    selectedIndex = 0;
    return;
  }
  if (currentState == SCREEN_ALL_TUNED) {
    currentState  = SCREEN_HOME;
    selectedIndex = 0;
    return;
  }
  if (currentState != SCREEN_HOME) {
    currentState  = getParentState(currentState);
    selectedIndex = 0;
  }
}

void UI::renderCurrentScreen() {
  switch (currentState) {
    case SCREEN_HOME:
      display.drawHomeScreen(selectedIndex);
      break;
    case SCREEN_SELECT_TUNING:
      display.drawSelectTuningScreen(selectedIndex);
      break;
    case SCREEN_SETTINGS:
      display.drawSettingsScreen(selectedIndex);
      break;
    case SCREEN_TUNING:
      display.drawTuningScreen(_currentNote, _targetNote, _currentString, _sharp);
      break;
    case SCREEN_PLUCK_PROMPT:
      display.drawPluckPromptScreen(_pluckOrdinal, _pluckStrLabel, _pluckNoteName, _pluckTargetHz);
      break;
    case SCREEN_ALL_TUNED:
      display.drawAllTunedScreen();
      break;
  }
}