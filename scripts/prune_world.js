const FS = require('fs');

const dir = FS.opendirSync('../res/map');

const map = FS.readFileSync('../res/map/world').toString().split('\n');

let prunable = [];

let dirent;
while ((dirent = dir.readSync()) != null) {
  if (dirent.isFile()) {
	let name = dirent.name;
	let dots = name.split('.');
	let ext = dots[dots.length - 1];
	if (ext == "actor")
	{
		let id = dots[dots.length - 2];
		if (!map.includes("map/" + id))
		{
			prunable.push(id);
		}
	}
  }
}

dir.closeSync();

let doPrune = process.argv[2] == "-prune";
console.log("== Actor Pruner ==");

if (!doPrune)
	console.log("There are " + prunable.length + " unused actors. To prune, run with \"-prune\".");

if (doPrune)
{
	console.log("Pruning " + prunable.length + " unused actors.");
	for (let pruned of prunable) {
		console.log("Obliterating actor " + pruned + "...");
		FS.unlinkSync("../res/map/" + pruned + ".actor");
	}
	console.log("Done!");
}