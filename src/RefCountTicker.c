#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Tick struct

typedef struct Tick {
	int referenceCount;
	time_t time;
	// Each tick object holds some state, here just 10MB of empty data
	char state[1024 * 1024 * 10];
} Tick;

Tick * newTick() {
	Tick * tick = calloc(1, sizeof(Tick));
	tick->referenceCount = 1;
	tick->time = time(NULL);
	return tick;
}

void freeTick(Tick * tick) {
	free(tick);
}

// Reference counting functions

Tick ** autoReleaseBuffer = NULL;
int autoReleaseBufferSize = 0;

Tick * makeAutoRelease(Tick * t) {
	if(autoReleaseBuffer == NULL) {
		autoReleaseBufferSize = 10;
		autoReleaseBuffer = calloc(autoReleaseBufferSize, sizeof(Tick *));
	}

	for(int i = 0; i < autoReleaseBufferSize; ++i) {
		if(autoReleaseBuffer[i] == NULL) {
			autoReleaseBuffer[i] = t;
			return t;
		}
	}

	// we couldn't find a free slot
	autoReleaseBuffer = realloc(autoReleaseBuffer, autoReleaseBufferSize * 2 * sizeof(Tick *));
	memset(autoReleaseBuffer + autoReleaseBufferSize, 0, autoReleaseBufferSize * sizeof(Tick *));
	autoReleaseBuffer[autoReleaseBufferSize] = t;
	autoReleaseBufferSize *= 2;
	return t;
}

Tick * retain(Tick * t) {
	if(t != NULL) {
		t->referenceCount++;
	}
	return t;
}

void release(Tick * t) {
	if(t != NULL) {
		t->referenceCount--;
		if(t->referenceCount <= 0) {
			freeTick(t);
		}
	}
}

void drainAutoReleaseBuffer() {
	for(int i = 0; i < autoReleaseBufferSize; ++i) {
		if(autoReleaseBuffer[i] != NULL) {
			release(autoReleaseBuffer[i]);
			autoReleaseBuffer[i] = NULL;
		}
	}
}

// Tick handler functions

void logTick(Tick * tick) {
	printf("Tick: %s", ctime(&tick->time));
}

Tick * lastTick = NULL;

void deltaTick(Tick * tick) {
	if(lastTick == NULL) {
		lastTick = retain(tick);
		return;
	}

	int delta = (int)(tick->time - lastTick->time);
	release(lastTick);
	lastTick = retain(tick);

	if(delta > 5) {
		printf("%d seconds passed between ticks\n", delta);
	}
}

// main

int main(void) {
	char c;
	while((c = getchar()) != EOF) {
		Tick * t = makeAutoRelease(newTick());
		logTick(t);
		deltaTick(t);
		drainAutoReleaseBuffer();
	}

	release(lastTick);
	drainAutoReleaseBuffer();
	free(autoReleaseBuffer);
}
