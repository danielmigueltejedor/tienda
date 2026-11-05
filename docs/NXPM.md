# NXPM

## Métodos

La interfaz `os.nodalix.Nxpm1` ofrece:

| Método | Entrada | Salida |
| --- | --- | --- |
| `ApiVersion` | | `u` |
| `ListSources` | | `aa{sv}` |
| `ListInstalled` | `s` fuente, vacía para todas | `aa{sv}` |
| `Search` | `s` texto | `aa{sv}` |
| `GetDetails` | `s` id | `a{sv}` |
| `ListUpdates` | | `aa{sv}` |
| `Install` | `s` id, `s` fuente | `s` transacción |
| `Remove` | `s` id, `s` fuente | `s` transacción |
| `Update` | `s` id | `s` transacción |
| `UpgradeSystem` | | `s` transacción |
| `GetTransaction` | `s` id | `a{sv}` |
| `Cancel` | `s` id | `b` |
| `GetHistory` | `u` límite | `aa{sv}` |

La señal `TransactionChanged` envía un `a{sv}` con el estado de la operación.

Los identificadores válidos son:

- `flatpak:user:app/org.ejemplo.App/x86_64/stable`
- `flatpak:system:runtime/org.freedesktop.Platform/x86_64/24.08`
- `pacman:nombre`

Cualquier otro texto se rechaza antes de tocar el sistema.

## Estados

Una transacción está en `pending`, `running`, `succeeded`, `failed` o `cancelled`. Solo se cancela con seguridad mientras sigue en `pending`. Una operación Flatpak ya empezada no se aborta: libflatpak, en la versión usada, no ofrece una cancelación que este servicio pueda garantizar.

Las operaciones se ejecutan de una en una.

## Flatpak

El backend usa `Flatpak.Installation` de la introspección GObject. Busca en las instalaciones de usuario y de sistema, instala y retira referencias del usuario, y detecta actualizaciones con `list_installed_refs_for_update`. Los cambios de la instalación de sistema no se aplican desde el servicio de usuario.

## Pacman

Las consultas usan `pyalpm` con la configuración que lee `pycman` desde `/etc/pacman.conf`, incluidos los repositorios y `IgnorePkg`. No se descarga la base de datos dentro de una búsqueda: se usan las bases que ya tiene el equipo.

`list_updates` compara la versión local con la de los repositorios sincronizados mediante `pyalpm.vercmp`.

Instalar o actualizar un solo paquete oficial desde la tienda está rechazado. La actualización oficial es `UpgradeSystem`, que llama a `transaction.sysupgrade(False)` y por tanto resuelve el conjunto completo. Ese camino solo se ejecuta como root. Desde la sesión, la tienda lo lanza con `pkexec` y el comando `nxpm privileged-upgrade`. Si la autorización no se concede, la operación termina en error y no cambia el sistema.

Antes de esa actualización se guarda en el estado del usuario un inventario `nombre versión`. Es un registro, no una instantánea de disco ni una vuelta atrás automática. No se asume Btrfs, Snapper ni ningún otro mecanismo.

## CLI

`nxpm` importa el motor. No reimplementa las operaciones ni llama a la tienda.

```bash
PYTHONPATH=nxpm/src python -m nxpm sources
PYTHONPATH=nxpm/src python -m nxpm search firefox
PYTHONPATH=nxpm/src python -m nxpm updates
PYTHONPATH=nxpm/src python -m nxpm list --source pacman
PYTHONPATH=nxpm/src python -m nxpm daemon
```

`nxpm check` escribe `~/.local/state/nxpm/updates.json` y, si hay avisos disponibles, muestra una notificación. No instala nada.

`nxpm install-session` copia el servicio de usuario y un temporizador de systemd que ejecuta `nxpm check` cada seis horas. El temporizador no queda activado hasta que se ejecute `systemctl --user enable --now nxpm-check.timer`.

## Errores

Los fallos usan códigos estables: `invalid-input`, `not-found`, `unsupported`, `authorization-required`, `disabled` y `failed`. En D-Bus se traducen a nombres `os.nodalix.Nxpm1.Error.*`.
