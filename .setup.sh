
b() {
  source .build.sh
}

r() {
  if [ ! -f ./.env ]; then
    echo "Error: .env file not found. Please create one from .env.example." >&2
    return 1
  fi
  docker run --rm -it --env-file ./.env -v "$(pwd)":/app growth-asymmetry-builder /app/build/"$@"
}