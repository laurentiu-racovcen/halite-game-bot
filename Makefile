.PHONY: build run clean

build:
	g++ -o resources/MyBot resources/MyBot.cpp

run:
	./resources/MyBot

clean:
	rm -f resources/MyBot