#include "../include.h"

// ****************************************************************************
//
//                        Picocalc I2C Keyboard Tester v6
//
// ****************************************************************************
// Implementuje protokol zjištěný z repozitáře:
// 1. Zapsat registr 0x09
// 2. Přečíst 2 byty (16-bit hodnota)

#define I2C_PORT        i2c1
#define PIN_SDA         6
#define PIN_SCL         7
#define TARGET_ADDR     0x1F

#define LOG_LINES 14
char log_buffer[LOG_LINES][40];

void LogPrint(const char* format, ...) {
	for (int i = 0; i < LOG_LINES - 1; i++) {
		memcpy(log_buffer[i], log_buffer[i+1], 40);
	}
	va_list args;
	va_start(args, format);
	vsnprintf(log_buffer[LOG_LINES-1], 40, format, args);
	va_end(args);
	// printf("%s\n", log_buffer[LOG_LINES-1]);
}

void DrawLog() {
	DrawClear();
	DrawText("I2C KBD TESTER v6 (0x09)", 0, 0, COL_YELLOW);
	int y = 20;
	for (int i = 0; i < LOG_LINES; i++) {
		u16 color = (i == LOG_LINES-1) ? COL_WHITE : COL_GRAY;
		DrawText(log_buffer[i], 0, y, color);
		y += 12;
	}
	DispUpdate();
}

void I2C_ClearBus() {
	// Reset sběrnice (pro jistotu)
	gpio_init(PIN_SDA); gpio_set_dir(PIN_SDA, GPIO_IN);
	gpio_init(PIN_SCL); gpio_set_dir(PIN_SCL, GPIO_OUT);
	for (int i = 0; i < 9; i++) {
		if (gpio_get(PIN_SDA)) break;
		gpio_put(PIN_SCL, 0); sleep_us(10);
		gpio_put(PIN_SCL, 1); sleep_us(10);
	}
	gpio_set_dir(PIN_SDA, GPIO_OUT);
	gpio_put(PIN_SDA, 0); gpio_put(PIN_SCL, 1); sleep_us(10);
	gpio_put(PIN_SDA, 1); sleep_us(10);
}

int main() {
	DeviceInit();
	for(int i=0; i<LOG_LINES; i++) log_buffer[i][0] = 0;

	// 1. Init HW
	I2C_ClearBus();
	i2c_init(I2C_PORT, 100 * 1000); // 100 kHz
	gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
	gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(PIN_SDA);
	gpio_pull_up(PIN_SCL);
	WaitMs(200);

	LogPrint("Startuji cteni reg 0x09...");
	DrawLog();

	while (True) {
		u8 rx_data[2] = {0, 0};
		u8 reg = 0x09;

		// KROK 1: Zapsat registr 0x09
		// true = nostop (posle Restart místo Stop, aby neztratil vlastnictví sběrnice)
		int w_ret = i2c_write_blocking(I2C_PORT, TARGET_ADDR, &reg, 1, true);

		if (w_ret < 0) {
			// Chyba zápisu
			// LogPrint("Err Write: %d", w_ret);
			// DrawLog();
			WaitMs(10);
			continue;
		}

		// KROK 2: Přečíst 2 byty
		int r_ret = i2c_read_timeout_us(I2C_PORT, TARGET_ADDR, rx_data, 2, false, 50000);

		if (r_ret == 2) {
			// Úspěšně přečteny 2 byty
			u16 key_code = (rx_data[1] << 8) | rx_data[0]; // Little Endian? Nebo Big? Zkusime oboji

			// Podle C++ kódu: if(buff!=0) return buff;
			if (key_code != 0) {
				// Máme stisk!
				char c = (char)rx_data[1]; // Předpokládáme, že spodní byte je ASCII
				LogPrint("KEY: 0x%04X (L:%02X H:%02X) '%c'", key_code, rx_data[0], rx_data[1], (c>32 && c<127)?c:'.');
				DrawLog();
			}
			// Pokud je 0, nic neděláme (klidový stav)

		} else {
			// Chyba čtení
			// LogPrint("Err Read: %d", r_ret);
			// DrawLog();
		}
		u8 key = KeyGet();
		if (key == KEY_Y) ResetToBootLoader();
		WaitMs(20); // Interval skenování
	}
}
