const FS = require('fs');

const dir = FS.opendirSync('.');

let dirent;
while ((dirent = dir.readSync()) != null) {
  if (dirent.isFile()) {
	let name = dirent.name;
	let dots = name.split('.');
	let ext = dots[dots.length - 1];
	if (ext == "actor")
	{
		let id = dots[dots.length - 2];


		let x = FS.readFileSync(name, 'utf-8');
		FS.writeFileSync(name, "$id " + id.toString() + "\n" + x);
		
	}
  }
}
