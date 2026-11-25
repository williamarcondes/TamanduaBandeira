# Tamanduá-Bandeira

Aventura de plataforma 2D original para Nintendo Switch homebrew. Você controla um tamanduá-bandeira em uma jornada por cenários inspirados na Mata de Poços de Caldas, na cidade e em um prédio de vidro. O projeto tem personagens, arte e música próprios; não usa executáveis nem dados de Little Fighter 2.

## Aventura

Explore três fases, colete grãos de café e enfrente um chefe no final de cada cenário:

1. **Mata de Poços de Caldas** — floresta, formigas e criaturas da mata; chefe Formiga Rainha.
2. **Poços de Caldas** — percurso urbano por ruas, praça e telhados; chefe Guardião da Praça.
3. **Prédio de vidro** — escritório de programação, plataformas elevatórias e bugs digitais; chefe Bug Supremo.

Grãos visíveis e escondidos contam para a aventura e abastecem o turbo, usado para rolar. Marcas no chão indicam onde cavar; corações recuperam vida. Cada chefe precisa receber três pisões e anda pela arena. A cabeça da Formiga Rainha funciona como mola. Na mata, espinhos de roseira cobrem um trecho do chão; no percurso urbano, um rio bloqueia a passagem; no prédio, cabos energizados fecham a rota pelo piso. As plataformas existentes permitem cruzar cada perigo. Os perigos, faíscas, projéteis, itens e plataformas têm animações leves para dar mais movimento às fases. O trecho do rio não tem chão sólido; cair nele reinicia o retorno por solo firme. Os chefes miram projéteis no jogador em baixa frequência: grão de milho na mata, espiga na cidade e pipoca no prédio. Depois de derrotar o chefe, pegue a garrafa-troféu de café para ver fogos e comemorar; a vitória usa um sprite único do tamanduá segurando a garrafa aberta e bebendo. O coração de vida tem arte própria no cenário e no HUD. Os golpes tocam um efeito curto; quando uma criatura é derrotada, aparecem cortes vermelhos e fumaça. A vitória final termina com **“Encontrou um café excelente!”**.

O herói começa com cinco vidas. Ao cair ou tocar em um monstro, a animação do anjinho aparece e o herói retorna perto do local, com alguns segundos de proteção. O progresso da fase é preservado. O início de cada fase tem um trecho seguro para aprender os controles.

## Controles

| Controle do Switch | Ação |
| --- | --- |
| Direcional ou analógico esquerdo | Mover |
| B | Pular e pisar nos inimigos e chefes |
| A | Cavar marcas no chão |
| Direcional para baixo | Agachar |
| Y | Rolar; consome turbo |
| + | Pausar e consultar o objetivo |

No teclado: setas ou A/D movem; Espaço ou Z pula; X cava; seta para baixo agacha; Shift ou Y rola; Esc pausa.

## Compilar para Nintendo Switch

### Requisitos

- Windows com Git Bash e Docker Desktop usando Linux Engine, ou Linux com Docker.
- Um estágio de instalação do SDL3 compilado para Nintendo Switch, copiado para build/switch/vendor/SDL/build-switch/stage. Ele deve conter include/SDL3 e lib/libSDL3.a. A pasta build/ é local e ignorada pelo Git.
- Para usar outra instalação, informe o caminho em SDL3_STAGE.

No Git Bash, execute:

~~~bash
bash tools/build-switch.sh
~~~

O script usa por padrão o estágio SDL3 local em build/switch/vendor/SDL/build-switch/stage; ele não usa arquivos da pasta antiga nem inclui essa dependência no repositório. Também prepara os recursos, compõe a música e compila o jogo com a imagem Docker do devkitPro fixada por digest. O NRO será criado em build/switch/TamanduaBandeira.nro.

Para instalar, copie o arquivo para SD:/switch/TamanduaBandeira/TamanduaBandeira.nro e abra-o pelo Homebrew Menu. O jogo não precisa do instalador original nem cria arquivos de Little Fighter 2.

## Arquivos do projeto

- src/main.cpp — lógica do jogo, fases, controles, interface e desenho.
- assets/source/ — imagens de origem dos cenários, personagens e ícone.
- assets/icon.jpg — ícone usado no NRO.
- tools/prepare_assets.py — prepara os sprites e cenários para o jogo.
- tools/compose_music.py — compõe a faixa original de guitarra, baixo e bateria.
- tools/compose_sfx.py — gera o efeito curto de impacto usado nos golpes.
- PROXIMA-ITERACAO.md — modelo pronto para descrever a próxima rodada de melhorias.
- tools/build-switch.sh — compilação para Nintendo Switch.

Os arquivos intermediários e produtos de compilação ficam em build/. A pasta dist/ e os binários do Switch estão no .gitignore e não devem entrar no commit.
