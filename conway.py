import bext
import time, random, sys

GENERATIONS = 3
PAUSE = 0.25
current_things = {}
next_things = {}

WIDTH, HEIGHT = bext.size()
WIDTH = WIDTH - 1
HEIGHT = HEIGHT - 1
HEIGHT = (HEIGHT - 1) * 2 - 1
# Leave a row free for "Press Ctrl-C..." message.

bext.clear()
for x in range(WIDTH):
    for y in range(HEIGHT):
        if random.randint(0, 1) == 0:
            next_things[x, y] = True


def update():
    global current_things, next_things, WIDTH, HEIGHT
    for i in range(1, GENERATIONS):
        current_things = next_things
        current_things_loc = current_things
        next_things = {}
        for x in range(WIDTH):
            for y in range(HEIGHT):
                # Get neighboring coordinates:
                left_coord = x - 1 % HEIGHT
                right_coord = x + 1 % HEIGHT
                top_coord = y - 1 % HEIGHT
                bottom_coord = y + 1 % HEIGHT

                # Get neighbors:
                top_left = (left_coord, top_coord) in current_things
                top = (x, top_coord) in current_things
                top_right = (right_coord, top_coord) in current_things
                left = (left_coord, y) in current_things
                right = (right_coord, y) in current_things
                bottom_left = (left_coord, bottom_coord) in current_things
                bottom = (x, bottom_coord) in current_things
                bottom_right = (right_coord, bottom_coord) in current_things
                num_neighbors = top_left + top + top_right + left + right + bottom_left + bottom + bottom_right

                # Set cell based on Conway's Game of Life rules:
                if current_things.get((x, y), False):
                    if num_neighbors in (2, 3):
                        next_things[x, y] = True
                else:
                    if num_neighbors == 3:
                        next_things[x, y] = True


def draw(step):
    # bext.fg('black')
    for x in range(WIDTH):
        for y in range(HEIGHT):
            if current_things.get((x, y), False):
                bext.goto(x, y)
                print('*', end='')


def main():
    global PAUSE
    step = 0
    # bext.bg('white')
    while True:
        try:
            update()
            bext.clear()
            draw(step)
            time.sleep(PAUSE)
            step += 1
        except KeyboardInterrupt:
            sys.exit()


if __name__ == '__main__':
    main()

