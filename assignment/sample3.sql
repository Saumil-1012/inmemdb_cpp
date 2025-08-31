CREATE TABLE students (id int, name str, grade str, score int);
CREATE TABLE courses (id int, name str, teacher str);

INSERT INTO students (id, name, grade, score) VALUES
                                                  (1, "Alice", "A", 95),
                                                  (2, "Bob", "B", 82),
                                                  (3, "Carol", "A", 98),
                                                  (4, "David", "C", 75),
                                                  (5, "Eve", "A", 92);

INSERT INTO courses (id, name, teacher) VALUES
                                            (1, "Math", "Dr. Smith"),
                                            (2, "Science", "Dr. Johnson"),
                                            (3, "History", "Dr. Brown");

SELECT * FROM students WHERE score >= 90;
SELECT name, grade FROM students WHERE grade = "A";
SELECT * FROM students WHERE score < 80;
SELECT * FROM students WHERE name != "Bob";

UPDATE students SET score = 97 WHERE name = "Alice";
SELECT name, score FROM students WHERE name = "Alice";

DELETE FROM students WHERE score < 80;
SELECT * FROM students;

SELECT students.name, courses.teacher
FROM students INNER JOIN courses ON students.id = courses.id;

SELECT * FROM students INNER JOIN courses ON students.id = courses.id;
