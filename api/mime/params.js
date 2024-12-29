export class MIMEParams extends Map {
  toString () {
    return Array
      .from(this.entries())
      .map((entry) => entry.join('='))
      .join(';')
  }
}

export default MIMEParams
