const path = require("path");
const fse = require("fs-extra");
const del = require("del");
const gulp = require("gulp");

const paths = {
  dist: "data/web",
};

function clean() {
  return del([paths.dist], { force: true });
}

function buildDeps() {
  const rootDir = path.resolve(__dirname, "./");
  const libDeps = path.resolve(__dirname, "./.pio/libdeps");
  if (fse.existsSync(libDeps)) {
    const envNames = fse.readdirSync(libDeps);
    for (const envName of envNames) {
      const libDir = path.resolve(libDeps, envName, "home-esp8266/data");
      if (fse.existsSync(libDir)) {
        fse.copySync(libDir, path.resolve(rootDir, "data"), {
          overwrite: true,
        });
      }
    }
  }
  return Promise.resolve();
}

const build = gulp.series(clean, buildDeps);

// export gulp tasks
exports.build = build;
exports.default = build;
