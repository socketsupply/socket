if [ "$(uname)" = "Linux" ] || [ "$(uname)" = "Darwin" ]; then
  env bash $@
else
  sh $@
fi

exit $?
