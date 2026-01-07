buddy_demo:
	gcc buddy.c main.c -o buddy_demo -g

test: test_buddy
	./test_buddy

test_buddy: buddy.c test_buddy.c
	gcc buddy.c test_buddy.c -o test_buddy -g -lm

clean:
	rm -f buddy_demo test_buddy

.PHONY: test clean
