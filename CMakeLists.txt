# Define a versão mínima do CMake
cmake_minimum_required(VERSION 3.13)

# Definições de padrões de compilação
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == Configuração automática do Raspberry Pi Pico SDK ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()

set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)

if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

# Define a placa como Raspberry Pi Pico W (caso seja Pico normal, mude para `pico`)
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Importa o SDK do Pico
include(pico_sdk_import.cmake)

# Nome do projeto
project(tarefa_interfaces C CXX ASM)

# Inicializa o SDK do Pico
pico_sdk_init()

# === Definição do Executável ===
add_executable(tarefa_interfaces
    tarefa_interfaces.c    
    main.c        # Código principal (renomeie caso seu arquivo não seja "main.c")
    ssd1306.c     # Driver do display OLED
)

# Define nome e versão do programa
pico_set_program_name(tarefa_interfaces "tarefa_interfaces")
pico_set_program_version(tarefa_interfaces "0.1")

# === Configuração do PIO ===
pico_generate_pio_header(tarefa_interfaces ${CMAKE_CURRENT_LIST_DIR}/ws2812b.pio)

# Habilita saída via USB e UART
pico_enable_stdio_uart(tarefa_interfaces 1)
pico_enable_stdio_usb(tarefa_interfaces 1)

# === Bibliotecas do Pico ===
target_link_libraries(tarefa_interfaces
    pico_stdlib       # Biblioteca padrão do Pico
    hardware_i2c      # Comunicação I2C (SSD1306)
    hardware_pio      # Comunicação PIO (WS2812B)
    hardware_clocks   # Controle de clocks
    hardware_gpio     # Controle de GPIOs (botões e LEDs)
)

# === Inclusão de diretórios ===
target_include_directories(tarefa_interfaces PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}  # Inclui arquivos do projeto
)

# Gera arquivos extras (binário, UF2, etc.)
pico_add_extra_outputs(tarefa_interfaces)
