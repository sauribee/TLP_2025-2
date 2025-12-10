# 📘 Referencia del Lenguaje .brik

El **lenguaje .brik** es un DSL (Domain Specific Language) diseñado específicamente para configurar juegos del Brick Game Engine sin necesidad de modificar código Python.

---

## 📖 Índice

1. [Sintaxis básica](#sintaxis-básica)
2. [Tipos de datos](#tipos-de-datos)
3. [Estructura de un archivo .brik](#estructura-de-un-archivo-brik)
4. [Secciones comunes](#secciones-comunes)
5. [Configuración de Snake](#configuración-de-snake)
6. [Configuración de Tetris](#configuración-de-tetris)
7. [Ejemplos completos](#ejemplos-completos)

---

## Sintaxis básica

### Comentarios

```brik
// Comentario de línea (estilo C++)
# Comentario de línea (estilo Python)
```

### Asignaciones

```brik
nombre = valor;
```

**Importante**: Cada asignación debe terminar con punto y coma (`;`).

### Secciones anidadas

```brik
seccion {
    subseccion {
        propiedad = valor;
    }
}
```

---

## Tipos de datos

### 1. Números

```brik
entero = 42;
decimal = 3.14;
negativo = -10;
```

### 2. Cadenas de texto (strings)

```brik
nombre = "Snake Game";
control = "left";
```

### 3. Colores (hexadecimales)

```brik
color_rojo = #FF0000;
color_verde = #00FF00;
color_azul = #0000FF;
color_custom = #2C8D3E;
```

### 4. Booleanos

```brik
activo = true;
desactivo = false;
```

### 5. Listas (arrays)

```brik
numeros = [1, 2, 3, 4, 5];
coordenadas = [10, 20];
colores = [#FF0000, #00FF00, #0000FF];
```

### 6. Listas anidadas (matrices)

```brik
matriz = [
    [1, 0, 0],
    [0, 1, 0],
    [0, 0, 1]
];
```

### 7. Identificadores (referencias)

```brik
available_pieces = [piece_I, piece_O, piece_T];
```

---

## Estructura de un archivo .brik

### Estructura mínima

```brik
game "nombre_del_juego" {
    // Configuraciones aquí
}
```

### Estructura completa

```brik
[tipo_juego] version [numero]

game "nombre_del_juego" {
    board {
        // Configuración del tablero
    }
    
    controls {
        // Configuración de controles
    }
    
    // Otras secciones específicas del juego
}
```

**Ejemplo**:

```brik
snake version 1.0

game "snake" {
    board {
        width = 20;
        height = 24;
    }
}
```

---

## Secciones comunes

### `board` - Configuración del tablero

Define las propiedades visuales y dimensiones del área de juego.

```brik
board {
    width = 20;          // Ancho en celdas
    height = 24;         // Alto en celdas
    cell_size = 20;      // Tamaño de cada celda en píxeles
    wrap = false;        // ¿Los bordes se conectan? (true/false)
    
    colors {
        background = #FFFFFF;   // Color de fondo
        grid       = #CCCCCC;   // Color de la grilla
        walls      = #555555;   // Color de las paredes
    }
}
```

### `controls` - Configuración de controles

Mapea las acciones del juego a teclas del teclado.

```brik
controls {
    right_mov = "right";   // Tecla flecha derecha
    left_mov  = "left";    // Tecla flecha izquierda
    down_mov  = "down";    // Tecla flecha abajo
    up_mov    = "up";      // Tecla flecha arriba
    pause     = "p";       // Tecla P para pausar
    restart   = "r";       // Tecla R para reiniciar
}
```

**Teclas disponibles**:
- Flechas: `"up"`, `"down"`, `"left"`, `"right"`
- Letras: `"a"` a `"z"`
- Espacio: `"space"`
- Enter: `"return"`
- Escape: `"escape"`

### `level` - Diseño del nivel

Define el mapa del nivel usando una matriz, donde:
- `0` = celda vacía (transitable)
- `1` = pared/obstáculo

```brik
level {
    grid = [
        [1,1,1,1,1,1,1,1,1,1],
        [1,0,0,0,0,0,0,0,0,1],
        [1,0,0,0,0,0,0,0,0,1],
        [1,0,0,0,0,0,0,0,0,1],
        [1,0,0,0,0,0,0,0,0,1],
        [1,0,0,0,0,0,0,0,0,1],
        [1,1,1,1,1,1,1,1,1,1]
    ];
}
```

---

## Configuración de Snake

### Sección `snake`

```brik
snake {
    tick_ms = 120;           // Velocidad del juego (ms por frame)
    initial_length = 1;      // Longitud inicial de la serpiente
    growth_per_apple = 3;    // Cuánto crece al comer una manzana
    spawn = "random";        // Dónde aparece: "random", "center", [x,y]
}
```

### Sección `portals`

Configura portales de teletransporte.

```brik
portals {
    random = true;           // Portales aleatorios (true) o fijos (false)
    num_pairs = 2;           // Número de pares de portales
    
    // Si random = false, define posiciones manualmente:
    p1_from = [3, 10];      // Portal 1: entrada
    p1_to   = [15, 10];     // Portal 1: salida
    
    p2_from = [4, 5];       // Portal 2: entrada
    p2_to   = [4, 15];      // Portal 2: salida
}
```

### Sección `rules_end_game`

Define las condiciones de fin de juego.

```brik
rules_end_game {
    on_out_of_bounds = "end";     // "end" o "wrap" o "ignore"
    on_self_collision = "end";    // "end" o "ignore"
    on_wall_collision = "end";    // "end" o "ignore"
}
```

### Sección `rules_speed_progression`

Configura la aceleración del juego.

```brik
rules_speed_progression {
    speedup_after_apple = 5;   // Acelera cada N manzanas
    min_tick_ms = 60;          // Velocidad máxima (no baja de esto)
}
```

### Sección `rules_spawning`

Controla la aparición de objetos.

```brik
rules_spawning {
    apple_respawn_ticks = 5;   // Ticks para que aparezca otra manzana
    max_apples = 10;           // Máximo de manzanas simultáneas
}
```

### Sección `rules_scoring`

Define los puntos.

```brik
rules_scoring {
    apple_points = 10;         // Puntos por comer una manzana
}
```

---

## Configuración de Tetris

### Sección `tetris`

```brik
tetris {
    tick_ms = 650;           // Velocidad de caída (ms)
    gravity = 1;             // Velocidad de caída en celdas/tick
    spawn = "top_center";    // Dónde aparecen las piezas
}
```

### Sección `ui`

```brik
ui {
    next_queue_length = 3;   // Cuántas piezas "next" mostrar
}
```

### Sección `pieces`

Define cada pieza de Tetris con sus rotaciones.

```brik
pieces {
    piece_I {
        color = "#75E689";
        rotations = [
            // Rotación 0: horizontal
            [
                [0,0,0,0],
                [1,1,1,1],
                [0,0,0,0],
                [0,0,0,0]
            ],
            // Rotación 1: vertical
            [
                [0,1,0,0],
                [0,1,0,0],
                [0,1,0,0],
                [0,1,0,0]
            ]
        ];
    }
    
    piece_O {
        color = "#7599E6";
        rotations = [
            [
                [0,1,1,0],
                [0,1,1,0],
                [0,0,0,0],
                [0,0,0,0]
            ]
        ];
    }
    
    // ... más piezas (T, J, L, S, Z)
}
```

### Lista de piezas disponibles

```brik
available_pieces = [piece_I, piece_O, piece_T, piece_J, piece_L, piece_S, piece_Z];
```

### Pieza bomba especial

```brik
pieces {
    piece_bomb {
        color = "#F08418";
        rotations = [
            [
                [1,0,0,0],
                [0,0,0,0],
                [0,0,0,0],
                [0,0,0,0]
            ]
        ];
        blast_radius = 1;    // Radio de explosión
    }
}

rules_for_bomb {
    blast_radius = 1;
    time_to_detonate = 3;    // Tiempo en segundos
    color = "#F08418";
}

rules_random_pieces {
    random_bombs = true;
    bomb_chance = 0.05;      // 5% de probabilidad
}
```

### Sección `rules_line_clear`

```brik
rules_line_clear {
    line_score = [40, 100, 300, 1200];  // Puntos por 1, 2, 3, 4 líneas
    combo_bonus = 50;                   // Bonus por combo
    level_multiplier = 1.3;             // Multiplicador por nivel
}
```

### Sección `rules_speed_levels`

```brik
rules_speed_levels {
    speed_increase = true;              // ¿Aumenta velocidad?
    level_up_each = 1000;               // Puntos para subir de nivel
    speed_multiplier = 1.3;             // Factor de aceleración
}
```

### Sección `rules_end_game`

```brik
rules_end_game {
    game_over = "stack_reaches_top";    // Condición de game over
}
```

---

## Ejemplos completos

### Snake básico

```brik
snake version 1.0

game "snake" {
    board {
        width = 20;
        height = 24;
        cell_size = 20;
        
        colors {
            background = #FFFFFF;
            snake_body = #2C8D3E;
            apple      = #F74848;
        }
    }
    
    controls {
        right_mov = "right";
        left_mov  = "left";
        down_mov  = "down";
        up_mov    = "up";
        pause     = "p";
        restart   = "r";
    }
    
    snake {
        tick_ms = 120;
        initial_length = 3;
        growth_per_apple = 1;
    }
    
    rules_scoring {
        apple_points = 10;
    }
}
```

### Tetris básico

```brik
tetris version 1.0

game "tetris" {
    board {
        width = 10;
        height = 20;
        
        colors {
            background = #000000;
            grid       = #333333;
        }
    }
    
    controls {
        left_mov   = "left";
        right_mov  = "right";
        super_drop = "down";
        rotate     = "up";
        pause      = "p";
    }
    
    tetris {
        tick_ms = 500;
        gravity = 1;
    }
    
    available_pieces = [piece_I, piece_O, piece_T];
}
```

---

## Validación y debugging

Para validar la sintaxis de un archivo `.brik`:

```bash
python dsl/brik_parser.py specs/tu_archivo.brik
```

Si hay errores, el parser mostrará:
- Línea y columna del error
- Token problemático
- Mensaje descriptivo del problema

---

## Mejores prácticas

1. **Usa comentarios** para documentar configuraciones complejas
2. **Mantén valores consistentes**: Si `board.width = 20` y `cell_size = 20`, la ventana será de 400px
3. **Prueba incrementalmente**: Empieza con configuraciones simples y añade complejidad gradualmente
4. **Respeta los límites**: Valores muy altos en `width`/`height` pueden causar lag
5. **Colores legibles**: Asegúrate de que haya contraste entre fondo y elementos del juego

---

## Errores comunes

| Error | Causa | Solución |
|-------|-------|----------|
| `Expected ';'` | Falta punto y coma al final de asignación | Agregar `;` al final |
| `Unexpected token` | Sintaxis incorrecta | Revisar estructura de llaves `{}` y corchetes `[]` |
| `Expected identifier` | Palabra clave incorrecta | Verificar nombres de secciones |
| Color no válido | Formato de color incorrecto | Usar formato `#RRGGBB` |

---

## Extensión futura

El lenguaje `.brik` está diseñado para ser extensible. En futuras versiones podrías:

- Definir power-ups personalizados
- Crear animaciones
- Configurar AI de enemigos
- Diseñar niveles procedurales

---

Para más información sobre cómo usar estos archivos en Python, consulta [API.md](API.md).
