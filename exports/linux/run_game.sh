GAME_ROOT=$(cd "${0%/*}" && echo $PWD)
GAME_EXE=new_winds
LD_LIBRARY_PATH="${GAME_ROOT}"/bin ${GAME_ROOT}/${GAME_EXE}