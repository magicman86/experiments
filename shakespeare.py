import bext
import time, random, sys

PAUSE = 0.05
MAX_POPULATION = 2000
MUTATION = 0.02
DESIRED = "what about longer strings such as this"
POSSIBLE_CHARACTERS = "abcdefghijklmnopqrstuvwxyz "

population = []
next_population = []
fitness_pool = []
fittest_index = 0
generations = 0


def random_character():
    return POSSIBLE_CHARACTERS[random.randint(0, len(POSSIBLE_CHARACTERS)-1)]


def generate_random():
    global DESIRED
    random_string = list("")
    for char in DESIRED:
        random_string.append(random_character())
    return random_string


def fitness(value):
    global DESIRED
    score = 0
    pos = 0
    for char in value:
        if DESIRED[pos] == char:
            score = score + 1
        pos = pos + 1
    score = pow(score, 2)
    return score


def child(parent1, parent2):
    midpoint = random.randint(0, len(parent1) - 1)

    genome1a = parent1[:midpoint]
    genome1b = parent2[midpoint:]
    genome2a = parent2[:midpoint]
    genome2b = parent1[midpoint:]

    possibilities = [genome1a + genome1b, genome2a + genome2b]
    index = random.randint(0, 1)

    return possibilities[index]


def mutate(value):
    global MUTATION
    pos = 0
    for char in value:
        if random.random() <= MUTATION:
            value[pos] = random_character()
        pos = pos + 1
    return value


def calculate_fitness_pool():
    global fitness_pool, fittest_index
    fitness_pool = []
    fittest_index = 0
    total_value = 0
    for i in range(0, len(population)):
        word = population[i]
        fitness_value = fitness(word)
        fitness_pool.append(fitness_value)
        if fitness_value > fitness_pool[fittest_index]:
            fittest_index = i


def pick_child():
    x = 0
    while x < 1000:
        index = random.randint(0, len(population) - 1)
        fitness_to_beat = random.randint(0, fitness_pool[fittest_index])
        if fitness_pool[index] >= fitness_to_beat:
            return population[index]

    return population[fittest_index]


def generate_new_children():
    global population, next_population
    next_population = []
    for i in range(0, MAX_POPULATION):
        parent1 = pick_child()
        parent2 = pick_child()
        next_population.append(child(parent1, parent2))


def update():
    global MAX_POPULATION
    calculate_fitness_pool()
    generate_new_children()


def draw():
    global DESIRED, generations
    most_fit = population[fittest_index]
    most_fit_score = fitness_pool[fittest_index]
    bext.clear()
    # bext.goto(0, 0)
    generations = generations + 1
    print("most fit: " + "".join(most_fit))
    print("generations: " + str(generations))
    return most_fit_score == fitness(DESIRED)


def main():
    global PAUSE, MAX_POPULATION, population, next_population
    step = 0

    for i in range(0, MAX_POPULATION):
        population.append(generate_random())

    while True:
        try:
            update()
            bext.clear()
            if draw():
                return
            population = next_population
            time.sleep(PAUSE)
            step += 1
        except KeyboardInterrupt:
            sys.exit()


if __name__ == '__main__':
    main()
