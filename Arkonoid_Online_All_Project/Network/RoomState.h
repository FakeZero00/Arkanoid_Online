#pragma once
// Waiting -> Playing -> TimeUp (1s grace: only events with timeMs <= limit) -> Ended
enum class RoomState {
	Waiting,
	Playing,
	TimeUp,
	Ended
};
