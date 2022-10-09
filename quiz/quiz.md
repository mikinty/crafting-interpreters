# Compilers Quiz

Questions I thought of as I went through this course.

1. Why is it hard for languages to support arbitrary identifier names (e.g. classes or variables with leading digits for example)

When you are parsing, you usually assume if you see a digit, that you can just assume you have a number for the rest of the lexeme. But if you have identifiers all of a sudden, now you have to look ahead and make sure you have a number before assuming you do. This is expensive.

It's not necessarily hard, it's just slower and restricting identifier names is not a terrible hinderance to developer experience.