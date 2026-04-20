savedcmd_eats.mod := printf '%s\n'   eats.o | awk '!x[$$0]++ { print("./"$$0) }' > eats.mod
