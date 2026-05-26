#!/usr/bin/env bash

ITERATIONS=50
STRESS_TEST_FILE="./stress_test2.txt"
CSV_FILE="benchmarks.csv"

# Scriviamo l'intestazione del CSV (sovrascrivendo il vecchio file)
printf "Mode, Mean(sec), Standard deviation(sec)\n" > "$CSV_FILE"

# Definiamo la funzione universale di benchmark
run_benchmark() {
    local mode_name="$1"  # Primo argomento (es. "Single-Thread")
    local extra_flag="$2" # Secondo argomento (es. "--multi" o vuoto)

    local sum=0
    local sum_squared=0
    local data=()

    echo "Running benchmark for $mode_name..."

    # Fase 1: Raccolta dati e calcolo della Somma
    for ((i=1; i <= ITERATIONS; i++))
    do
        # Eseguiamo il comando passando l'eventuale flag extra
        local cur_output
        cur_output=$(./a.out "$STRESS_TEST_FILE" $extra_flag | grep "Execution Time:" | grep -Eo "[0-9]+\.[0-9]+")

        data[i]=$cur_output
        sum=$(echo "$sum + $cur_output" | bc -l)
    done

    # Calcolo della Media
    local mean
    mean=$(echo "$sum / $ITERATIONS" | bc -l)

    # Fase 2: Calcolo della Varianza (Sum of Squares)
    for ((i=1; i <= ITERATIONS; i++))
    do
        local diff
        local squared
        diff=$(echo "${data[i]} - $mean" | bc -l)
        squared=$(echo "$diff ^ 2" | bc -l) # <--- CORRETTO: aggiunto -l
        sum_squared=$(echo "$sum_squared + $squared" | bc -l)
    done

    # Calcolo della Deviazione Standard
    local std_dev
    std_dev=$(echo "sqrt($sum_squared / $ITERATIONS)" | bc -l)

    # Appendiamo i risultati formattati direttamente nel CSV
    printf "%s, %.9f, %.9f\n" "$mode_name" "$mean" "$std_dev" >> "$CSV_FILE"
}

# --- ESECUZIONE DEL SCRIPT ---

# Lanciamo il benchmark per il Single-Thread
run_benchmark "Single-Thread" ""

# Lanciamo il benchmark per il Multi-Thread
run_benchmark "Multi-Thread" "--multi"

echo "Benchmark completato! I risultati sono in $CSV_FILE"