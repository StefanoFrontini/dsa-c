# Dichiariamo la versione minima di CMake richiesta
cmake_minimum_required(VERSION 3.10)

# Diamo un nome al nostro progetto
project(SnakeGame)

# Diciamo a CMake di creare un eseguibile chiamato "snake"
# unendo questi due file sorgenti
add_executable(snake snake.c platform_mac.c)

# Aggiungiamo i flag di compilazione (i nostri soliti -W -Wall)
target_compile_options(snake PRIVATE -W -Wall)