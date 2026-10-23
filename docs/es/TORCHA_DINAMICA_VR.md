# Antorcha Dinámica de Mano (1H Club) para VR

## 1. Visión General
Esta característica transforma la antorcha o garrote de una mano (`clb`, `spc`) en una fuente de iluminación dinámica desacoplada del cuerpo que responde en tiempo real a la mano y muñeca del jugador en Realidad Virtual, proyectando luz viva y parpadeante en mazmorras cerradas (Catacumbas, Cloacas, Tumbas de Tal Rasha) y permitiendo el combate cuerpo a cuerpo contundente (1H Club swing).

Todo el diseño se ciñe estrictamente a las **Golden Rules**:
- **Cero ediciones a `.d2s`:** No se crean objetos ni códigos de objeto nuevos (`weapons.txt` intacto).
- **Compatibilidad 100% con juego de escritorio:** Un personaje puede jugarse indistintamente en VR o en monitor plano sin corrupción.
- **Solución canónica D2:** Los garrotes (`clb`, `spc`) cualifican como antorchas ardientes al imbuirse con fuego legítimo (rubí engarzado, runa Ral, garrote único *Felloak*, afijos mágicos de fuego o hechizo *Enchant* de la hechicera).

---

## 2. Fase 1: Tracking y Modelo Matemático
Implementado en [steamvr/torch_light.h](../../steamvr/torch_light.h):

- **Geometría y Offset de Agarre:**
  - `DefaultShaftOffset = {0.0f, 0.32f, -0.06f}`: punto emisor de la llama situado en la cabeza del garrote (~32 cm hacia arriba en el eje del puño OpenXR y ligeramente inclinado al frente).
  - Función `ComputeTorchTip`: calcula la posición 3D exacta de la punta de la antorcha en el espacio de juego XR a partir de la pose del mando.
- **Inercia y Arrastre de la Llama:**
  - `ComputeFlameDirection`: calcula el vector de inclinación de la llama combinando la flotabilidad ascendente `{0, 1, 0}` con el arrastre por velocidad lineal del mando (`-velocity * dragFactor`).
- **Simulación de Fuego Orgánico (Flicker):**
  - `ComputeFlameFlickerWave`: combinación de armónicos senoidales primos (3.1 Hz, 7.3 Hz, 13.7 Hz, 23.9 Hz) que generan turbulencia continua en el rango `[-1.0, 1.0]` sin saltos artificiales.
  - `ComputeFlickerRadius`: modulación suave del radio de iluminación (`18.0f ± 6%`).
  - `ComputeFlickerColor`: modulación cromática de temperatura de fuego (rojo/dorado cálido).
- **Mapeo a Coordenadas de Motor D2R:**
  - `LocalToWorldGrid`: convierte el vector relativo de la mano en coordenadas de mundo de D2R (`heroX`, `heroY`, `heroZ`).

---

## 3. Fase 2: Lógica de Cualificación e Inspección de Equipo
Implementado en [steamvr/torch_light.h](../../steamvr/torch_light.h) y [steamvr/interactions.h](../../steamvr/interactions.h):

- `HasTorchInHand(Snapshot, hand, outFlags)`: verifica en cada fotograma si el conjunto activo de armas contiene un garrote válido con fuego en la mano indicada (0 = principal, 1 = secundaria).
- **Criterios de Activación:**
  - Engarces con gemas de fuego o runas (`socketed > 0`).
  - Garrote único *Felloak* (`quality == 7` y código `"clb"`).
  - Hechizo de Hechicera *Enchant* activo en el jugador (`effects.Has(16)`).
  - Override para pruebas en modo desarrollo (`developer && clb`).

---

## 4. Fase 3: Inyección en Render Pipeline y Shader ReShade
Implementado en [vr/vrcam.cpp](../../vr/vrcam.cpp) y [reshade/D2R_DepthFog.fx](../../reshade/D2R_DepthFog.fx):

- **Asignación del Slot 0:** El slot 0 del array uniforme `GameLightPos[16]` y `GameLightCol[16]` se reserva para la antorcha sostenida en la mano.
- **Transformación de Coordenadas:** La punta del garrote obtenida mediante `skel::MeleeWeapon(wpose, hand)` se proyecta relativa al héroe y suelo: `(tipX - heroX, tipY - floorY, tipZ - heroZ)`.
- **Escala de Radio Exacta:** `ComputeShaderLightRadius` normaliza el alcance respecto al baseline del techo (`CeilTorchRadius`), garantizando la distancia exacta configurada.
- **Intensidad Inmune a Distancia:** Se asigna intensidad $1.0$ (`col[3] = 1.0f`), impidiendo que la antorcha se desvanezca por alejarse del centro.
- **Activación de Bóveda y Paredes:** `CeilTorches` y `GameLightsOn` se activan automáticamente al portar la antorcha, proyectando luz viva en techos y muros de roca incluso en salas sin antorchas de mundo.
- **Acomodo de Luces Estáticas:** Los slots restantes (1 a 15) se rellenan con las luces de mundo y monstruos de `worldobj::Lights`.
- **Configuración (`d2r_pcvr.ini`):**
  - `[torch] enabled=1`: Activa/desactiva la iluminación de antorcha en VR.
  - `[torch] radius=18.0`: Alcance de la luz en unidades de mundo.
  - `[torch] flicker=1`: Activa el parpadeo senoidal multifrecuencia.
  - `[torch] warmth=0.65`: Ajuste de temperatura cromática hacia naranja fuego.

---

## 5. Menú de Desarrollador (Dev Scenario)
- Opción **"Torch kit"** en la página **EQUIPMENT** del menú DEV (Choice ID 41).
- Genera en el inventario un garrote de una mano (`clb` - Club normal, nivel 1) para pruebas inmediatas en Blood Moor y Catacumbas.

---

## 6. Validación Automatizada
- Test nativo `torch-light-test`: valida cálculos de offset, inclinación por velocidad, límites del parpadeo, conversión espacial, reconocimiento de códigos, reglas de cualificación, escala de radio de shader, y empaque del slot 0.
- Test `dev-scenarios-test`: valida la inclusión del kit de antorcha en la página de equipamiento.
