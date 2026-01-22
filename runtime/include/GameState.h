#pragma once

enum class GameState {
    Running,
    StartOfFrame,
    NextFrame,
    PreviousFrame,
    JumpToFrame,
    RestartFrame,
    EndFrame,
	RestartApplication,
	EndApplication
};

