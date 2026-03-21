// Script Handles Button Presses anc Converts to State Changes

#include "ui.h"
#include "display.h"
#include <string.h>

static Display display;
static UIState currentState  = SCREEN_HOME;
static UIState previousState = SCREEN_HOME;

// Get whether tuner should be running or not
bool UI::isTuningActive() { return tuningActive; }

void UI::stopTuning() {
  tuningActive  = false;
  currentState  = SCREEN_SELECT_TUNING;
  selectedIndex = 0;
}

// For Collecting Selected Key Data for External Use
const char* UI::getSelectedNote() { return noteOptions[selectedNoteIndex]; }

const char* UI::getSelectedType() { return typeOptions[selectedTypeIndex]; }

int UI::getSelectedNoteIndex() { return selectedNoteIndex; }

int UI::getSelectedTypeIndex() { return selectedTypeIndex; }

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

static int getOptionCountForState(UIState state) {
  switch (state) {
    case SCREEN_HOME:
      return 3;
    case SCREEN_SELECT_TUNING:
      return 3;
    case SCREEN_SETTINGS:
      return 3;
    case SCREEN_TUNING:
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

    case SCREEN_SETTINGS:
      return SCREEN_HOME;

    case SCREEN_HOME:
    default:
      return SCREEN_HOME;
  }
}

void UI::initUI() {
  currentState  = SCREEN_HOME;
  previousState = SCREEN_HOME;
  selectedIndex = 0;
}

void UI::handleLeft() {
  if (currentState == SCREEN_SELECT_TUNING && isEditingField) {
    if (selectedIndex == 1) {
      selectedNoteIndex--;
      if (selectedNoteIndex < 0) selectedNoteIndex = NOTE_COUNT - 1;
    } else if (selectedIndex == 2) {
      selectedTypeIndex--;
      if (selectedTypeIndex < 0) selectedTypeIndex = TYPE_COUNT - 1;
    }
    return;
  }

  int count = getOptionCountForState(currentState);
  selectedIndex--;
  if (selectedIndex < 0) selectedIndex = count - 1;
}

void UI::handleRight() {
  if (currentState == SCREEN_SELECT_TUNING && isEditingField) {
    if (selectedIndex == 1) {
      selectedNoteIndex++;
      if (selectedNoteIndex >= NOTE_COUNT) selectedNoteIndex = 0;
    } else if (selectedIndex == 2) {
      selectedTypeIndex++;
      if (selectedTypeIndex >= TYPE_COUNT) selectedTypeIndex = 0;
    }
    return;
  }

  int count = getOptionCountForState(currentState);
  selectedIndex++;
  if (selectedIndex >= count) selectedIndex = 0;
}

void UI::handleSelect() {
  switch (currentState) {
    case SCREEN_HOME:
      if (selectedIndex == 0) {
        currentState = SCREEN_SELECT_TUNING;
      } else if (selectedIndex == 1) {
        currentState = SCREEN_SETTINGS;
      }
      selectedIndex = 0;
      break;

    case SCREEN_SELECT_TUNING:
      if (isEditingField) {
        isEditingField = false;
      } else {
        if (selectedIndex == 0) {  // Start selected
          tuningActive  = true;
          currentState  = SCREEN_TUNING;
          selectedIndex = 0;
        } else if (selectedIndex == 1 || selectedIndex == 2) {
          isEditingField = true;
        }
      }
      break;

    case SCREEN_TUNING:
      break;

    case SCREEN_SETTINGS:
      break;
  }
}

void UI::renderCurrentScreen() {
  switch (currentState) {
    case SCREEN_HOME:
      display.drawHomeScreen(selectedIndex);
      break;
    case SCREEN_SELECT_TUNING:
      display.drawSelectTuningScreen(selectedIndex, noteOptions[selectedNoteIndex],
          typeOptions[selectedTypeIndex], isEditingField);
      break;
    case SCREEN_SETTINGS:
      display.drawSettingsScreen(selectedIndex);
      break;
    case SCREEN_TUNING:
      display.drawTuningScreen(_currentNote, _targetNote, _currentString, _sharp);
      break;
  }
}

void UI::handleBack() {
  if (currentState == SCREEN_SELECT_TUNING && isEditingField) {
    isEditingField = false;
    return;
  }

  if (currentState == SCREEN_TUNING) {
    tuningActive  = false;
    currentState  = SCREEN_SELECT_TUNING;
    selectedIndex = 0;
    return;
  }

  if (currentState != SCREEN_HOME) {
    currentState  = getParentState(currentState);
    selectedIndex = 0;
  }
}
