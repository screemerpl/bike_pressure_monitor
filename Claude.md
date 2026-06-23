# CLAUDE.md

Przewodnik kontekstowy dla modelu AI (Claude) pracującego nad repozytorium `bike_pressure_monitor`.

## 1) Cel projektu
- Firmware TPMS na `ESP32-C3` z UI na `LVGL`.
- Obsługa dwóch trybów:
  - `MODE_BIKE` (2 czujniki: przód/tył)
  - `MODE_CAR` (4 czujniki: FL/RL/FR/RR)
- Dane czujników przychodzą po BLE i są renderowane na ekranie.

## 2) Najważniejsze pliki
- `main/Application.cpp/.h` — główny orchestrator aplikacji (init, pętle, aktualizacje UI, wejście przycisku, konfiguracja).
- `main/State.cpp/.h` — singleton globalnego stanu (mapa czujników, adresy, targety ciśnień, cache ostatnich odczytów).
- `main/UIController.cpp/.h` — lifecycle LVGL (tick/task) i przejścia ekranów.
- `main/UIBikeController.cpp/.h` — render ekranu głównego dla motocykla.
- `main/UICarController.cpp/.h` — render ekranu głównego dla auta.
- `main/ConfigManager.cpp/.h` — zapis/odczyt NVS (JSON przez cJSON).
- `main/TPMSScanCallbacks.cpp/.h` — parse BLE reklam i aktualizacja `State`.
- `main/UI/*` — kod generowany przez SquareLine (traktować ostrożnie).

## 3) Aktualny ważny feature (persist + fallback)
Wdrożone zachowanie:
- Ostatnie poprawne ciśnienie jest zapisywane do NVS jako:
  - `sensor_last_psi_0..3`
- Po restarcie/braku świeżych danych UI pokazuje ostatni zapis zamiast `---`.
- Gdy czujnik jest sparowany, ale czeka na synchronizację, miga jego ikona BT.
- Jeśli miga BT, wartość ciśnienia NIE miga.

Miejsca implementacji:
- `main/State.h`: `LastSensorReading`, `getLastReading()`, `setLastReading()`
- `main/Application.cpp`:
  - load cache z NVS w `loadConfiguration()`
  - throttled zapis w `persistLastSensorReadingIfNeeded()`
  - przekazanie fallback/sync-state do `UIBikeController` i `UICarController`
- `main/UIBikeController.cpp` i `main/UICarController.cpp`:
  - fallback render ciśnienia z cache
  - BT blink dla `awaitingSync`
  - wyłączenie blinku ciśnienia przy BT blink

## 4) Mapa indeksów sensorów
Stałe w `main/State.h`:
- Bike:
  - `SENSOR_BIKE_FRONT = 0`
  - `SENSOR_BIKE_REAR = 1`
- Car:
  - `SENSOR_CAR_FRONT_LEFT = 0`
  - `SENSOR_CAR_REAR_LEFT = 1`
  - `SENSOR_CAR_FRONT_RIGHT = 2`
  - `SENSOR_CAR_REAR_RIGHT = 3`

Uwaga na mapowanie UI C1..C4 (w kodzie auta):
- C1=FL, C2=RL, C3=FR, C4=RR.

## 5) Zasady pracy na kodzie
- Zmiany rób minimalnie i lokalnie (bez refactoru „przy okazji”).
- Nie edytuj wygenerowanych plików `main/UI/*`, jeśli nie ma wyraźnej potrzeby.
- Dla logiki runtime preferuj edycję `Application`, `State`, `UIBikeController`, `UICarController`.
- Zachowuj obecny styl C++ (czytelne nazwy, brak zbędnych komentarzy inline).
- Nie zmieniaj publicznych API, jeśli nie musisz.

## 6) Build / uruchamianie (Windows PowerShell)
Standardowo (po aktywacji ESP-IDF):
```powershell
idf.py set-target esp32c3
idf.py build
idf.py -p COMx flash monitor
```

Jeśli `idf.py` nie jest dostępne:
- uruchom terminal ESP-IDF (ESP-IDF PowerShell), albo
- aktywuj środowisko ESP-IDF skryptem dostarczonym przez instalator.

## 7) Typowe pułapki
- Brak `idf.py`/`cmake` w PATH ≠ błąd kodu, tylko środowiska.
- Łatwo pomylić kolejność kół w trybie auta (FL/RL/FR/RR).
- `lv_async_call` i aktualizacje UI muszą iść w kontekście LVGL.
- Zbyt częsty zapis do NVS zużywa flash — używaj throttlingu (już jest w kodzie).

## 8) Checklist przy zmianach TPMS/UI
1. Czy mapowanie sensor index -> widget jest poprawne?
2. Czy fallback (`last_psi`) działa dla bike i car?
3. Czy przy `awaitingSync` miga BT, a nie ciśnienie?
4. Czy long-press reset czyści powiązane klucze NVS?
5. Czy `get_errors` nie pokazuje nowych błędów?

## 9) Co warto sprawdzać przed commitem
- `main/Application.cpp`
- `main/State.h`
- `main/UIBikeController.cpp/.h`
- `main/UICarController.cpp/.h`

I upewnij się, że nie wrzucasz niepowiązanych zmian (szczególnie z `main/UI/` i assetów).

## 10) Prompt templates dla Claude

Poniżej gotowe prompty do szybkiego użycia. Wystarczy podmienić dane w nawiasach `<>`.

### A) Bugfix UI (bike/car)
```text
Pracujesz w repo `bike_pressure_monitor`.
Napraw bug: <opisz objaw>.
Kontekst:
- tryb: <bike/car>
- ekran: <main/pair/splash>
- oczekiwane zachowanie: <...>

Wymagania:
1) Znajdź root cause, nie rób obejścia.
2) Zmień minimalny zakres plików.
3) Nie modyfikuj wygenerowanych `main/UI/*`, chyba że to konieczne.
4) Po zmianach sprawdź błędy (`get_errors`) i pokaż diff + krótkie uzasadnienie.
```

### B) BLE parser / czujniki TPMS
```text
Przeanalizuj przepływ BLE w `TPMSScanCallbacks` i napraw problem: <opis>.

Wymagania:
- Sprawdź rozróżnianie Type1/Type2.
- Zweryfikuj mapowanie adresów do `State::getData()`.
- Zachowaj kompatybilność obecnego API.
- Dodaj diagnostyczne logi tylko tam, gdzie pomagają w debugowaniu.
- Podaj, które scenariusze zostały pokryte i czego nie da się zweryfikować bez hardware.
```

### C) NVS / migracja konfiguracji
```text
Dodaj migrację konfiguracji NVS dla klucza/formatu: <stary_format> -> <nowy_format>.

Wymagania:
1) Implementacja w `ConfigManager`/`Application::loadConfiguration`.
2) Backward compatibility: jeśli stary klucz istnieje, migruj i zachowaj wartość.
3) Nie kasuj danych użytkownika bez wyraźnej potrzeby.
4) Dodaj bezpieczne defaulty i log migracji.
5) Opisz plan rollback i ryzyko.
```

### D) Feature: fallback ostatniego pomiaru
```text
Rozszerz feature fallback (`sensor_last_psi_X`) o: <nowa reguła>.

Wymagania:
- Aktualizuj bike i car spójnie.
- Jeśli miga BT (awaiting sync), ciśnienie ma pozostać statyczne.
- Ogranicz częstotliwość zapisu do NVS (throttle).
- Podaj dokładnie, które pliki zmieniono i dlaczego.
```

### E) Refactor kontrolowany
```text
Zrób mały refactor w: <plik/klasa>, bez zmiany zachowania runtime.

Wymagania:
1) Brak zmian funkcjonalnych.
2) Zachowaj sygnatury publiczne.
3) Uporządkuj tylko lokalny obszar kodu.
4) Pokaż przed/po dla kluczowego fragmentu i uzasadnij czytelność.
```

### F) Szybka diagnostyka build environment
```text
Pomóż zdiagnozować środowisko build dla ESP-IDF na Windows PowerShell.

Objaw: `idf.py` lub `cmake` nie jest znalezione.
Wymagania:
- Podaj kroki diagnozy i naprawy w kolejności.
- Daj komendy PowerShell do skopiowania.
- Nie zakładaj konkretnej ścieżki instalacji, podaj warianty.
```

### G) Prompt „pełny task” (najbardziej uniwersalny)
```text
Repo: `bike_pressure_monitor`
Cel: <co ma zostać osiągnięte>

Kryteria akceptacji:
- <kryterium 1>
- <kryterium 2>
- <kryterium 3>

Ograniczenia:
- Minimalny zakres zmian.
- Bez ruszania wygenerowanych plików `main/UI/*` (chyba że konieczne).
- Zachować kompatybilność istniejących flow bike/car.

Wykonanie:
1) Krótki plan.
2) Implementacja.
3) Walidacja przez `get_errors`.
4) Podsumowanie: co zmieniono, ryzyka, dalsze kroki.
```

## 11) Dobre praktyki promptowania w tym repo
- Zawsze podawaj, czy problem dotyczy `MODE_BIKE` czy `MODE_CAR`.
- Podawaj oczekiwane zachowanie UI przy braku sygnału (`---`, fallback, blink BT).
- Jeśli issue dotyczy czujników, podaj przykładowe adresy MAC i timing (np. „brak update > 200 ms”).
- Przy zgłoszeniach NVS podawaj klucze, których dotyczy zmiana.
- Proś o „minimal diff” i „root cause fix”.

## 12) DON’T (czego nie ruszać bez wyraźnej potrzeby)
- Nie edytuj wygenerowanych plików z `main/UI/*` tylko dlatego, że „ładniej wygląda” — zmieniaj je wyłącznie przy realnej potrzebie funkcjonalnej.
- Nie mieszaj niepowiązanych zmian w jednym tasku (np. logo/theme, assety, porządki formatowania) razem z fixem TPMS/UI.
- Nie zmieniaj mapowania kół ani indeksów sensorów bez aktualizacji całego flow (`State`, `Application`, `UIBikeController`, `UICarController`).
- Nie usuwaj istniejących kluczy NVS ani danych użytkownika bez migracji/backward compatibility.
- Nie dodawaj częstych zapisów do NVS w pętli UI/BLE bez throttlingu.
- Nie deklaruj „build OK”, jeśli nie został faktycznie wykonany w aktywnym środowisku ESP-IDF.
- Nie commituj zmian w assetach/UI, jeśli task dotyczył wyłącznie logiki C++.
- Nie modyfikuj publicznych API kontrolerów, jeśli da się osiągnąć cel lokalną zmianą implementacji.

## 13) Definition of Done
Task uznajemy za zamknięty tylko, gdy spełnione są wszystkie poniższe punkty:
- Problem rozwiązany w `root cause`, nie obejściem.
- Zakres zmian jest minimalny i dotyczy tylko potrzebnych plików.
- Dla zmian TPMS/UI sprawdzone są oba tryby: `MODE_BIKE` i `MODE_CAR` (jeśli dotyczy).
- Dla stanów braku danych potwierdzone zachowanie: fallback ostatniego pomiaru + poprawny blink BT.
- Nie ma nowych błędów w `get_errors` dla zmodyfikowanych plików.
- Nie ma niepowiązanych zmian w wygenerowanych plikach `main/UI/*` i assetach.
- Jeśli build nie został wykonany, status jest jasno opisany jako „niezweryfikowany środowiskowo”.
- Podsumowanie zmian zawiera: co zmieniono, dlaczego, ryzyka i następny krok.
