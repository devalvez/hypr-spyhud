# spyhud

Plugin de decorações de janela para o Hyprland, no estilo HUD "SPYHUNTER":
barra de título cinza-grafite com aba amarela, título em caixa alta e fonte
monoespaçada, régua de ticks, LEDs de status, botão de fechar quadrado e
colchetes amarelos nos 4 cantos da placa. A janela sem foco fica toda em cinza
(sem amarelo).

> **Status:** o código foi escrito contra os headers do branch principal do
> Hyprland (série 0.56, API com `Render::CRenderContext`), mas ainda não foi
> testado em uma instalação real. Se algo não compilar ou não renderizar como
> esperado, abra uma issue com a saída do `hyprctl version` e o log de erro.

## O que o plugin faz

- Barra de título reservada no topo (a janela é empurrada para baixo).
- Aba amarela + marcador, título em maiúsculas, ticks, 3 LEDs e botão de fechar.
- Colchetes nos 4 cantos da placa (barra + janela + borda).
- Janela com foco em amarelo; sem foco em cinza.
- Arrastar a barra move a janela; clicar no quadrado à direita fecha.

O que **não** faz: fundo de mapa/câmera, painéis flutuantes de dados e retículos
da imagem de referência. O plugin só decora janelas.

## Requisitos

- Hyprland com os headers instalados (`pkg-config --modversion hyprland`).
- Compilador compatível com o usado no Hyprland (GCC 15+ ou clang recente,
  C++26).
- `make`, `pkg-config`, e as dependências de desenvolvimento do Hyprland
  (`pixman`, `libdrm`, `libinput`, `libudev`, `wayland`, `xkbcommon`).

O plugin precisa ser compilado **contra a mesma versão** do Hyprland que está
rodando. Após atualizar o Hyprland, recompile.

## Instalação

### Opção A: hyprpm (recomendado)

O hyprpm instala a partir de um repositório git.

1. Publique o plugin em um repositório (por exemplo, no GitHub):

   ```bash
   git init && git add . && git commit -m "spyhud"
   git remote add origin https://github.com/SEU_USUARIO/spyhud.git
   git branch -M main && git push -u origin main
   ```

2. Instale e ative:

   ```bash
   hyprpm update                                      # baixa os headers do Hyprland instalado
   hyprpm add https://github.com/SEU_USUARIO/spyhud   # clona e compila
   hyprpm enable spyhud                               # ativa
   hyprpm list                                        # confere o estado
   ```

3. Para carregar sempre ao iniciar a sessão:

   ```lua
   -- hyprland.lua
   hl.exec_cmd("hyprpm reload -n")
   ```

   ```ini
   # hyprland.conf (hyprlang)
   exec-once = hyprpm reload -n
   ```

Depois de qualquer atualização do Hyprland, rode `hyprpm update` de novo.
Não use `hl.plugin.load` para o mesmo plugin se ele já está ativo no hyprpm,
senão ele carrega duas vezes.

Para ver o log de compilação em caso de erro: `hyprpm -v add <url>`.

### Opção B: compilação manual

```bash
make                  # gera spyhud.so
# ou
cmake -B build && cmake --build build
```

Carregue em tempo de execução:

```bash
hyprctl plugin load $PWD/spyhud.so
hyprctl plugin unload $PWD/spyhud.so
```

Ou carregue pela configuração:

```lua
-- hyprland.lua
hl.plugin.load("/caminho/para/spyhud.so")
```

```ini
# hyprland.conf (hyprlang)
plugin = /caminho/para/spyhud.so
```

## Configuração

Recarregue com `hyprctl reload` depois de editar. Não precisa recompilar para
mudar cores, fonte e tamanhos.

### hyprland.lua

```lua
hl.config({
    general = {
        gaps_in     = 10,
        gaps_out    = 18,
        border_size = 2,
        col = {
            active_border   = "rgba(5a5e64ff)",
            inactive_border = "rgba(26282bff)",
        },
    },

    decoration = {
        rounding         = 0,
        active_opacity   = 1.0,
        inactive_opacity = 0.92,

        -- "placa" deslocada atrás da janela
        shadow = {
            enabled      = true,
            range        = 0,
            render_power = 1,
            offset       = "6 6",
            color        = "rgba(0e0f10cc)",
        },

        blur = { enabled = false },
    },

    plugin = {
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

            col = {
                panel          = "rgba(34373bff)",
                panel_inactive = "rgba(2a2c2fff)",
                line           = "rgba(5a5e64ff)",
                accent         = "rgba(f2c400ff)",  -- amarelo
                dim            = "rgba(6b6f75ff)",
                text           = "rgba(d8dadcff)",
            },
        },
    },
})
```

Se você já tem blocos `general`/`decoration` no seu `hyprland.lua`, mescle os
valores neles em vez de repetir a chave.

### hyprland.conf (hyprlang)

O arquivo `spyhud.conf` deste repositório tem um bloco pronto para copiar. Resumo
das opções do plugin:

```ini
plugin {
    spyhud {
        enabled = true
        bar_height = 22
        text_size = 9
        font = JetBrains Mono
        bracket_length = 14
        bracket_thickness = 2
        ticks = true
        leds = true
        show_close = true

        col.panel          = rgba(34373bff)
        col.panel_inactive = rgba(2a2c2fff)
        col.line           = rgba(5a5e64ff)
        col.accent         = rgba(f2c400ff)
        col.dim            = rgba(6b6f75ff)
        col.text           = rgba(d8dadcff)
    }
}
```

### Opções (`plugin:spyhud:`)

| opção | tipo | padrão | descrição |
| --- | --- | --- | --- |
| `enabled` | bool | `true` | liga/desliga as decorações |
| `bar_height` | int | `22` | altura da barra de título (px) |
| `text_size` | int | `9` | tamanho do texto do título |
| `font` | string | `JetBrains Mono` | fonte do título (veja `fc-list`) |
| `bracket_length` | int | `14` | comprimento dos colchetes dos cantos |
| `bracket_thickness` | int | `2` | espessura dos colchetes |
| `ticks` | bool | `true` | régua de ticks na barra |
| `leds` | bool | `true` | três quadradinhos de status |
| `show_close` | bool | `true` | botão de fechar |
| `col.panel` | cor | `34373b` | fundo da barra com foco |
| `col.panel_inactive` | cor | `2a2c2f` | fundo da barra sem foco |
| `col.line` | cor | `5a5e64` | linha de brilho no topo da barra |
| `col.accent` | cor | `f2c400` | destaque (amarelo); cinza quando sem foco |
| `col.dim` | cor | `6b6f75` | detalhes secundários e janela sem foco |
| `col.text` | cor | `d8dadc` | cor do título |

Cores aceitam `"rgba(RRGGBBAA)"` ou inteiro `0xAARRGGBB`.

Exemplos de tema: ciano = `col.accent = rgba(00e5ffff)`; verde de terminal =
`col.accent = rgba(39ff14ff)`.

### O que fica fora do plugin

Estas partes do visual são do próprio Hyprland:

- Cor e espessura da borda: `general:col.active_border`,
  `general:col.inactive_border`, `general:border_size`.
- Sombra deslocada (efeito de painéis empilhados): `decoration:shadow`.
- Cantos retos: `decoration:rounding = 0`.

## Solução de problemas

- **Erro de versão ao carregar** (`Version mismatch`): o plugin foi compilado
  contra headers de outra versão do Hyprland. Rode `hyprpm update` e recompile.
- **Erros de compilação com `Window.hpp` ou `m_renderData`**: seus headers são de
  uma versão diferente da do código. Mande a saída de `hyprctl version`.
- **Opções do plugin ignoradas**: confira `hyprctl configerrors`. Em Lua, as
  chaves `col.*` ficam aninhadas em `col = { ... }`.
- **Plugin não aparece**: `hyprctl plugin list` ou `hyprpm list`.

## Arquivos

| arquivo | função |
| --- | --- |
| `main.cpp` | inicialização, opções de configuração, eventos |
| `HudDeco.cpp/.hpp` | decoração: layout, desenho, mouse |
| `HudPassElement.cpp/.hpp` | elemento do render pass |
| `globals.hpp` | estado global e configuração |
| `Makefile`, `CMakeLists.txt` | build |
| `hyprpm.toml` | manifesto para o hyprpm |
| `spyhud.conf` | exemplo de configuração em hyprlang |

