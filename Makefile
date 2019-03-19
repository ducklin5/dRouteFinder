zips:
	cd server && make clean
	cd client && make clean
	trash a2part2.zip
	zip -r a2part2.zip client server README
