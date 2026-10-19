# spyhud

Plugin de decorações de janela para Hyprland **0.56.0**, no estilo HUD "SPYHUNTER":
barra de título cinza-grafite com aba amarela, título em caixa alta monoespaçado,
régua de ticks, LEDs de status, botão de fechar quadrado e colchetes amarelos nos
4 cantos da placa. Janela inativa fica em cinza (sem amarelo).

## Build

Precisa dos headers do Hyprland **0.56.0** instalados (`pkg-config --modversion hyprland` → `0.56.0`).
Compile com o mesmo compilador/versão usado no Hyprland (GCC 15+/clang, C++26-capable).

    make            # gera spyhud.so
    # ou
    cmake -B build && cmake --build build

## Uso

    hyprctl plugin load $PWD/spyhud.so

ou no `hyprland.conf`: `plugin = /caminho/spyhud.so`. Copie o bloco de `spyhud.conf`
(bordas, sombra deslocada, rounding 0 e opções `plugin:spyhud:*`).

## Opções (`plugin:spyhud:`)

enabled, bar_height(22), text_size(9), font, bracket_length(14), bracket_thickness(2),
ticks, leds, show_close, col.panel, col.panel_inactive, col.line, col.accent, col.dim, col.text.

Arrastar a barra move a janela; clicar no quadrado à direita fecha.

#### Exemplo de personalizado

```lua
spyhud = {
	enabled           = true,
	bar_height        = 22,
	text_size         = 9,
	font              = "JetBrains Mono",
	bracket_length    = 14,
	bracket_thickness = 2,
	ticks             = true,
	leds              = true,
	show_close        = true,

	col               = {
		panel          = "rgba(34373bff)",
		panel_inactive = "rgba(2a2c2fff)",
		line           = "rgba(5a5e64ff)",
		accent         = "rgba(f2c400ff)", -- amarelo; troque aqui
		dim            = "rgba(6b6f75ff)",
		text           = "rgba(d8dadcff)",
	},
}
```
