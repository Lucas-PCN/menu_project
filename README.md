# Desenvolvimento de um menu interativo em Sistemas Embarcados

Bem-vindo ao projeto de Sistemas Embarcados utilizando a placa BitDogLab! Este repositório contém a implementação de um menu interativo no display SSD1306 (OLED), controlado por um joystick. As funcionalidades do sistema incluem a execução de três programas distintos: controle de LEDs com o joystick, acionamento de um buzzer e controle de um LED RGB via PWM.

## Tecnologias Utilizadas
- Linguagem C
- Pico SDK
- Placa BitDogLab com Raspberry Pi Pico W
- Display OLED SSD1306 (via protocolo I2C)
- Joystick analógico
- LED RGB
- Buzzer

## Estrutura do Projeto
O projeto foi desenvolvido com base nos exemplos disponibilizados no repositório da BitDogLab:
- [Joystick LED](https://github.com/BitDogLab/BitDogLab-C/tree/main/Joystick_led)
- [Buzzer PWM](https://github.com/BitDogLab/BitDogLab-C/tree/main/buzzer_pwm1)
- [LED RGB PWM](https://github.com/BitDogLab/BitDogLab-C/tree/main/PWM_LED_0)

## Funcionalidades
### Menu Interativo
O menu interativo permite navegar entre as opções utilizando o joystick:
- Movimentar o eixo **Y** para navegar para cima e para baixo.
- Pressionar o botão do joystick para selecionar uma opção.
- Pressionar o botão do joystick novamente para retornar ao menu principal.

### Opções do Menu
1. **Joystick LED**: Permite controlar os LEDs da placa movimentando o joystick.
2. **Tocar Buzzer**: Aciona um buzzer e toca a melodia do tema de Star Wars.
3. **Ligar LED RGB**: Controla a intensidade do LED RGB usando PWM.

## Configuração do Hardware
Os componentes devem ser conectados conforme a tabela abaixo:

| Componente  | Pino da Placa |
|-------------|--------------|
| Display OLED (SDA)  | 14 |
| Display OLED (SCL)  | 15 |
| LED Vermelho        | 13 |
| LED Azul           | 12 |
| LED Verde          | 11 |
| Buzzer            | 21 |
| Joystick (X)      | 27 |
| Joystick (Y)      | 26 |
| Joystick (Botão) | 22 |

## Como Executar o Projeto
1. Configure o ambiente de desenvolvimento com o **Pico SDK**.
2. Clone este repositório:
   ```sh
   git clone https://github.com/seu-usuario/seu-repositorio.git
   cd seu-repositorio
   ```
3. Compile o projeto:
   ```sh
   mkdir build && cd build
   cmake ..
   make
   ```
4. Carregue o binário gerado na placa Raspberry Pi Pico.
5. Reinicie a placa e utilize o joystick para navegar no menu.

## Simulação pronta no Wokwi
- Acesse [Wokwi](https://wokwi.com/projects/421969844378492929).

## Licença
Este projeto está sob a licença MIT. Veja o arquivo LICENSE para mais detalhes.

---

Projeto desenvolvido para a disciplina de Sistemas Embarcados.

