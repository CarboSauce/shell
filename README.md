# Grynszpan

Grynszpan (według [synonim.net](https://synonim.net/inaczej-pow%C5%82oka), synonim powłoki)
jest to powłoka systemowa implementująca podstawowe funkcje takie jak:

- Potoki między procesami
- Przekierowanie pliku na standardowe wyjście/wejście
- Asynchroniczne wywoływanie komend poprzez znak & na końcu komendy

Składnia przypomina składnie powłók POSIX:

```bash
cmd1 arg1 ... | cmd2 arg1 ... | ... < input > output & # Komentarz
```

Przykład:

```bash
echo -e "bbb\naaa\nccc\naaa" >| /tmp/test.txt
cat /tmp/test.txt | sort | uniq
```

Wyjście:

```
aaa
bbb
ccc
```

## Szczegółowy opis

Powłoka obsługuje zmiane katalogu przy użyciu komendy `cd`. Przy wysłaniu sygnału SIGQUIT bądz wpisaniu komendy `history` wyświetla się historia.

Historia przechowuje ostatnie 20 poleceń, oraz jest zapisywana do pliku .history w katalogu domowym użytkownika.

Na końcu każdej komendy znak `&` uruchamia dana komende w tle.

Wynik każdej z komend można przekierować do pliku przy użyciu jednego z trzech sposobów:

- `>` \- Tworzy plik oraz przekierowuje standardowe wyjście na dany plik pod warunkiem że dany plik nie istnieje.
- `>|` \- Funkcjonuje tak jak `>` lecz nadpisuje dany plik jeśli już istnieje.
- `>>` \- Funkcjonuje tak jak `>` lecz dodaje tekst na koniec jeśli plik już istnieje.

Przykład:
```bash
echo 123 > /tmp/test.txt
echo "Wystąpi błąd" > /tmp/test.txt
echo "Trzeba użyc >|" >| /tmp/test.txt
echo "Dodanie linii do pliku" >> /tmp/test.txt
```

Komendy można lączyc przy użyciu `|` który przekierowuje standardowe wyjście lewej komendy na standardowe wyjście komendy po prawej.

Przykład: 

```bash
ls | grep main.c
```

Powłoka poprawnie obsługuje [Shebang](https://pl.wikipedia.org/wiki/Shebang) dzięki czemu można zastosować Grynszpan do prostych skryptów.

Komenda `exit` konczy prace shella oraz czeka na zakończenie pod procesów wykonywanych asynchronicznie.

Dodatkowo można użyc komend `export` oraz `unexport` do odpowiednio dodawania oraz usuwania zmiennych srodowiskowych.

Przykład:

```bash
export ZMIENNA WARTOSC
export -o ZMIENNA "NADPISANIE WARTOSCI"
unexport ZMIENNA
```

W celu przekazania do komendy specjalnych znaków (np. `> | < "` oraz spacja) należy użyc "backslash".

## Kompilacja

W celu budowania projektu należy wykorzystać `make`

```bash
make run # buduje i uruchamia projekt
RELEASE= make run # kompilacja z optymalizacjami
```

## Dokumentacja funkcji

Kluczową funkcją w projekcie jest `parse_line`.

```c
int parse_line(parser_result* res, const char* line);
```
Funkcja ta parsuje linię podaną na standardowym wejściu w liste komend z argumentami oraz dane atrybuty całej komendy np. czy komenda powinna zostać wywołana asynchronicznie.

Funkcja zwraca wynik w pierwszym parametrze wyjściowym z typem `parser_result`. 

W przypadku błędów funkcja zwraca wartość 1 po czym obiekt wyjściowy jest w stanie nieokreślonym. Szczegółowe błędy są zwracane na standardowym wyjściu błedów (stderr).

W przypadku poprawnego wykonania funkcja zwraca 0 oraz obiekt zawiera dane o podanej komendzie.

Funkcji nie wolno używać przy obsłudze sygnałów gdyż alokuje pamięć oraz potencjalnie wypisuje błedy przy użyciu fprintf.

Pomyslnie utworzony obiekt należy zwolnić przy użyciu funkcji:

```c
void parser_result_dealloc(parser_result* in);
```

Struktury:

```c
typedef enum cmd_attributes {
	ATTRIBUTE_NONE   = 0,
	// przekierowanie wyjscia poprzez znak >>
	ATTRIBUTE_APPEND = 1,
	// przekierowanie wyjscia poprzez znak >|
	ATTRIBUTE_TRUNC  = 2,
	// przekierowanie wyjscia poprzez znak >
	ATTRIBUTE_EXCL   = 4,
	// komenda zawiera potok
	ATTRIBUTE_PIPE   = 8,
	// komenda zawiera przekierowanie wyjscia
	ATTRIBUTE_STDOUT = 16,
	// komenda zawiera przekierowanie wejscia
	ATTRIBUTE_STDIN  = 32
} cmd_attributes;

typedef struct shell_cmd {
    // liczba argumentów
	int argc;
	// argumenty zakonczone NULLem
	char** argv;
} shell_cmd;

typedef struct cmd_list {
    // tablica z komendami
	shell_cmd* commands;
	// rozmiar tablicy komend
	int size;
} cmd_list;

struct parser_result {
    // pole opisujące listę komend
	cmd_list cmdlist;
	// nazwa pliku do przekierowania standardowego wejscia
	// NULL jesli linia nie zawiera przekierowania
	char* stdinfile;
	// nazwa pliku do przekierowania standardowego wyjscia
	// NULL jesli linia nie zawiera przekierowania
	char* stdoutfile;
    // atrybuty komend
    // np. czy do przekierowania został użyty >> czy >
	cmd_attributes attrib; 
    // czy komenda jest asynchroniczna
	int is_async; 
} parser_result;
```

