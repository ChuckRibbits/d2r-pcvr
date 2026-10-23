# Antorcha Dinámica de Mano (1H Club) para VR

## 1. Visión General
Esta característica transforma la antorcha o garrote de una mano (`clb`) en una fuente de iluminación dinámica desacoplada del cuerpo que responde en tiempo real a la mano y muñeca del jugador en Realidad Virtual, proyectando luz viva y parpadeante en mazmorras cerradas (Catacumbas, Cloacas, Tumbas de Tal Rasha) y permitiendo el combate cuerpo a cuerpo contundente (1H Club swing).

## 2. Fase 1: Tracking y Modelo Matemático
Se ha implementado el módulo [torch_light.h](../../steamvr/torch_light.h):

- **Geometría y Offset de Agarre:**
  - `DefaultShaftOffset = {0.0f, 0.32f, -0.06f}`: punto emisor de la llama situado en la cabeza del garrote (~32 cm hacia arriba en el eje del puño OpenXR y ligeramente inclinado al frente).
  - Función `ComputeTorchTip`: calcula la posición 3D exacta de la punta de la antorcha en el espacio de juego XR a partir de la pose del mando.
- **Inercia y Arrastre de la Llama:**
  - `ComputeFlameDirection`: calcula el vector de inclinación de la llama combinando la flotabilidad ascendente `{0, 1, 0}` con el arrastre por velocidad lineal del mando (`-velocity * dragFactor`).
- **Simulación de Fuego Orgánico (Flicker):**
  - `ComputeFlameFlickerWave`: combinación de armónicos senoidales no múltiplos (3.1 Hz, 7.3 Hz, 13.7 Hz, 23.9 Hz) que generan turbulencia continua en el rango `[-1.0, 1.0]` sin saltos artificiales.
  - `ComputeFlickerRadius`: modulación suave del radio de iluminación (`18.0f ± 6%`).
  - `ComputeFlickerColor`: modulación cromática de temperatura de fuego (rojo/dorado cálido).
- **Mapeo a Coordenadas de Motor D2R:**
  - `LocalToWorldGrid`: convierte el vector relativo de la mano en coordenadas de mundo de D2R (`heroX`, `heroY`, `heroZ`).

## 3. Menú de Desarrollador (Dev Scenario)
- Se ha añadido la opción **"Torch kit"** en la página **EQUIPMENT** del menú DEV (Choice ID 41).
- Genera en el inventario un garrote de una mano (`clb` - Club normal, nivel 1) listo para equipar tanto en mano principal como en mano secundaria (dual-wield con Bárbaro o mano libre).

## 4. Validación Automatizada
- Test nativo `torch-light-test`: valida cálculos de offset, inclinación por velocidad, límites del parpadeo, conversión espacial y reconocimiento de códigos de garrotes.
- Test `dev-scenarios-test`: valida la inclusión del kit de antorcha en la página de equipamiento y su persistencia en el gestor de transacciones.
- **132 / 132 tests de CTest aprobados.**

