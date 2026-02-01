# Setup Instructions - Secrets Management

Your LoRa Gateway project uses a secrets management system to keep sensitive credentials (WiFi passwords, MQTT credentials) out of the git repository.

## How It Works

1. **`secrets_example.h`** - Template file showing what secrets you need (committed to git)
2. **`secrets.h`** - Your actual secrets file (gitignored, NOT committed)
3. **`Config.h`** - Includes `secrets.h` at compile time

## First-Time Setup

### 1. Create Your Secrets File

Copy the example file:

```bash
cd include
cp secrets_example.h secrets.h
```

### 2. Edit `secrets.h` with Your Credentials

```cpp
#pragma once

#define WIFI_SSID "YourWiFiName"
#define WIFI_PASSWORD "YourPassword"

#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "lora_gateway"
#define MQTT_USERNAME "mqtt_user"       // Leave empty if no auth
#define MQTT_PASSWORD "mqtt_password"   // Leave empty if no auth
```

### 3. Build and Upload

```bash
platformio run -e nodemcu-32s --target upload
```

## Important Notes

⚠️ **`secrets.h` is gitignored** - It will NOT be committed to git
⚠️ **Never add `secrets.h` to git** - Your credentials will be leaked
✅ **`secrets_example.h` is in git** - Share this with team members as template

## Sharing Your Project

When you push to git:
- ✅ `secrets_example.h` is included (helps others set up)
- ❌ `secrets.h` is excluded (your secrets stay safe)

Others can:
1. Clone the project
2. Copy `secrets_example.h` to `secrets.h`
3. Fill in their own credentials
4. Build and run

## If You Accidentally Committed Secrets

If secrets were already committed before this setup:

```bash
# Remove the file from git history
git rm --cached include/Config.h
git commit -m "Remove secrets from git history"

# Or use BFG Repo-Cleaner for entire history cleanup
# https://rtyley.github.io/bfg-repo-cleaner/
```

## File Structure

```
LoRaGateway-ESP/
├── include/
│   ├── Config.h               (includes secrets.h)
│   ├── secrets_example.h      (template - in git)
│   ├── secrets.h              (your secrets - GITIGNORED)
│   └── ... (other headers)
└── .gitignore                 (excludes secrets.h)
```

## Testing Your Setup

1. Build the project: `platformio run -e nodemcu-32s`
2. Check that `secrets.h` is in `.gitignore`: `git check-ignore include/secrets.h`
3. Verify it shows as gitignored (should return the path)

## Environment-Specific Secrets

For multiple environments (home, office, test):

**Option 1: Multiple Secrets Files**
```
secrets_home.h
secrets_office.h
secrets_test.h
```

Then in `Config.h`:
```cpp
#ifdef ENVIRONMENT_HOME
  #include "secrets_home.h"
#elif ENVIRONMENT_OFFICE
  #include "secrets_office.h"
#endif
```

Build with: `platformio run -e nodemcu-32s -D ENVIRONMENT_HOME`

**Option 2: Environment Variables**
Use PlatformIO's `platformio.ini` with environment variables:
```ini
[env:huzzah]
build_flags = 
  -D WIFI_SSID=\"$WIFI_SSID\"
  -D WIFI_PASSWORD=\"$WIFI_PASSWORD\"
```

Then set environment variables before building:
```bash
export WIFI_SSID="MyWiFi"
export WIFI_PASSWORD="MyPassword"
platformio run -e nodemcu-32s
```

## More Information

- [Git .gitignore Documentation](https://git-scm.com/docs/gitignore)
- [Protecting Secrets in Arduino Projects](https://docs.arduino.cc/cloud/iot-cloud/tutorials/secrets)
- [PlatformIO Build Flags](https://docs.platformio.org/en/latest/projectconf/build_options/index.html)
